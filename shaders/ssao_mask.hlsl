// Screen-space ambient occlusion for NierReplicantGFX.
//
// Replaces the game's compute shader gen_ssao_mask_default_c and keeps its interface (constant buffer,
// depth map, output), so the game's blur and composite passes use the result as before.
//
// Every pixel takes 8 directions x 6 steps and averages over the samples
//     saturate(cos(elevation above the tangent plane) - bias) * saturate(1 - d^2 / r^2),
// with the game's formula and sample spacing, so the strength matches the original shader.
// Samples that tell nothing about the surroundings are left out of the average:
//   - samples outside the screen;
//   - samples on something far in front of the surface (over one radius above its tangent plane and
//     nearer the camera), which covers whatever lies behind it.
// Direction rotation and step offset follow a 4x4 pattern; the game's blur pass smooths it out.
//
// STRENGTH multiplies the final exponent. tools/gen_ao_shader.py locates it in the bytecode and the
// plugin writes the value from the ini there.

#ifndef STRENGTH
#define STRENGTH 1.0
#endif

cbuffer CbGenSsaoMaskCompute : register(b0)
{
    float4x4 cb_mtx_view;
    float4 cb_uv_to_view;
    float2 cb_inv_size;
    float cb_far_clip;
    float cb_near_clip;
    float4 cb_out_tex_size;                 // AO mask: width, height, 1 / width, 1 / height
    float4 cb_in_tex_size;                  // depth map: the same
    float cb_radius_ss;                     // radius in pixels at view depth 1
    float cb_bias;
    float cb_intensity;                     // exponent
    float cb_neg_inv_r2;                    // -1 / (view-space radius)^2
    float cb_multiplier;
    float cb_ao_scale;
    float cb_inv_near_clip_feed_distance;
};

Texture2D<float> depthMap : register(t0);   // view depth / far clip
RWTexture2D<float2> aoMask : register(u0);  // x: AO (1 = unoccluded), y: view depth / (far - near)

#define DIRECTIONS 8
#define STEPS 6
#define TWO_PI 6.28318531

// 4x4 Bayer order: neighbouring pixels get distant rotations
static const uint BAYER[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

float3 view_pos(int2 t, float depth)
{
    float2 uv = (float2(t) + 0.5) * cb_in_tex_size.zw;
    return depth * float3((uv.x * 2.0 - 1.0) * cb_uv_to_view.x, (1.0 - uv.y * 2.0) * cb_uv_to_view.y, -cb_far_clip);
}

float3 load_pos(int2 t)
{
    return view_pos(t, depthMap.Load(int3(t, 0)));
}

bool inside(int2 t, int2 size)
{
    return all(t >= 0) && all(t < size);
}

// Tangent along `axis`: the shorter one-sided difference, so a silhouette next to the pixel does not tilt
// the normal. A side outside the depth map is not used.
float3 tangent(int2 t, int2 axis, float3 p, int2 size)
{
    float3 a = load_pos(t + axis) - p, b = p - load_pos(t - axis);
    bool use_b = !inside(t + axis, size) || (inside(t - axis, size) && dot(b, b) < dot(a, a));
    return use_b ? b : a;
}

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (any(id.xy >= (uint2)cb_out_tex_size.xy))
        return;
    int2 size = (int2)(uint2)cb_in_tex_size.xy;
    float2 pos = (float2(id.xy) + 0.5) * cb_out_tex_size.zw * float2(size);
    int2 t = clamp(int2(pos), 0, size - 1);
    float3 p = load_pos(t);
    float radius = min(abs(cb_radius_ss / p.z), cb_radius_ss);   // pixels

    float occlusion = 0.0;
    [branch] if (radius >= 1.0) {
        // the y axis of the depth map points down, so tangent(y) x tangent(x) faces the camera
        float3 n = cross(tangent(t, int2(0, 1), p, size), tangent(t, int2(1, 0), p, size));
        n *= rsqrt(max(dot(n, n), 1e-30));

        uint cell = BAYER[(id.x & 3) + (id.y & 3) * 4];
        float offset = ((reversebits(cell) >> 28) + 0.5) / 16.0;
        float2 dir, turn;
        sincos((cell + 0.5) * (TWO_PI / (16.0 * DIRECTIONS)), dir.y, dir.x);
        sincos(TWO_PI / DIRECTIONS, turn.y, turn.x);
        // the game samples at 1 + (0.5 .. 8.5) * spacing pixels
        float spacing = max(radius / 9.0, 1.0);
        float step = spacing * 8.0 / STEPS, start = 1.0 + 0.5 * spacing;
        float inv_r = sqrt(-cb_neg_inv_r2);
        float2 origin = float2(t) + 0.5;

        float sum = 0.0, weight = 0.0;
        [unroll] for (int i = 0; i < DIRECTIONS; i++) {
            [unroll] for (int k = 0; k < STEPS; k++) {
                int2 q = int2(floor(origin + dir * (start + (k + offset) * step)));
                float3 d = load_pos(q) - p;
                float d2 = dot(d, d), h = dot(d, n);
                float o = saturate(h * rsqrt(d2) - cb_bias) * saturate(d2 * cb_neg_inv_r2 + 1.0);
                // 0 up to one radius above the plane, 1 from two radii above and one radius nearer
                // the camera; the falloff of such a sample is already 0, so only its weight changes
                float hidden = saturate(h * inv_r - 1.0) * saturate(d.z * inv_r);
                float known = inside(q, size) ? 1.0 - hidden : 0.0;
                sum += o * known;
                weight += known;
            }
            dir = float2(dir.x * turn.x - dir.y * turn.y, dir.x * turn.y + dir.y * turn.x);
        }
        // with fewer than 1/8 of the samples known the result leans towards unoccluded
        occlusion = sum / max(weight, DIRECTIONS * STEPS / 8.0) * cb_multiplier * saturate(radius - 1.0);
    }

    float ao = saturate(1.0 - occlusion * cb_ao_scale);
    if (cb_inv_near_clip_feed_distance > 0.0) {
        float f = saturate(-p.z * cb_inv_near_clip_feed_distance);
        float w = min(ao - f + 1.0, 1.0);
        ao = f * (ao - w) + w;
    }
    aoMask[id.xy] = float2(pow(saturate(ao), cb_intensity * STRENGTH), -p.z / (cb_far_clip - cb_near_clip));
}
