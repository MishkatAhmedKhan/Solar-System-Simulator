#pragma once

const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 model, view, projection;
uniform vec3 lightPos;
out vec3 vNormal, vWorldPos, vViewDir;
void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    mat3 nm = mat3(transpose(inverse(model)));
    vNormal = normalize(nm * aNormal);
    vec3 camPos = (inverse(view) * vec4(0,0,0,1)).xyz;
    vViewDir = normalize(camPos - vWorldPos);
    gl_Position = projection * view * wp;
})glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec3 vNormal, vWorldPos, vViewDir;
out vec4 FragColor;
uniform vec4 objectColor;
uniform bool isGrid, GLOW, NEBULA;
uniform float ambient;
uniform vec3 lightPos;
uniform float uTime;

// ── Procedural Simplex Noise (Seamless 3D) ──
vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x*34.0)+1.0)*x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
  const vec2  C = vec2(1.0/6.0, 1.0/3.0);
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

  vec3 i  = floor(v + dot(v, C.yyy));
  vec3 x0 = v - i + dot(i, C.xxx);

  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min(g.xyz, l.zxy);
  vec3 i2 = max(g.xyz, l.zxy);

  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy;
  vec3 x3 = x0 - 0.5;

  i = mod289(i);
  vec4 p = permute(permute(permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0));

  float n_ = 1.0/7.0;
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);
  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_);

  vec4 x = x_ * ns.x + ns.yyyy;
  vec4 y = y_ * ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4(x.xy, y.xy);
  vec4 b1 = vec4(x.zw, y.zw);

  vec4 s0 = floor(b0) * 2.0 + 1.0;
  vec4 s1 = floor(b1) * 2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
  vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

  vec3 p0 = vec3(a0.xy, h.x);
  vec3 p1 = vec3(a0.zw, h.y);
  vec3 p2 = vec3(a1.xy, h.z);
  vec3 p3 = vec3(a1.zw, h.w);

  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;

  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot(m*m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

float fbm(vec3 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 7; i++) { v += a * (snoise(p)*0.5+0.5); p *= 2.03; a *= 0.48; }
    return v;
}
float fbm3(vec3 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 4; i++) { v += a * (snoise(p)*0.5+0.5); p *= 2.01; a *= 0.5; }
    return v;
}

void main() {
    if (isGrid) { FragColor = objectColor; return; }

    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vWorldPos);
    vec3 V = normalize(vViewDir);
    vec3 H = normalize(L + V);

    // ── BACKGROUND NEBULA (Giant procedurally generated cosmic dust cloud) ──
    if (NEBULA) {
        // Map based purely on viewing direction so it acts like a limitless skybox
        vec3 dir = -normalize(V); 

        // Create an angled band mapping the Milky Way core (tilt matches scene.cpp tilt = 1.05)
        float tilt = 1.05;
        float ct = cos(-tilt), st = sin(-tilt);
        vec3 rDir = vec3(dir.x, dir.y * ct - dir.z * st, dir.y * st + dir.z * ct);
        
        float lat = asin(rDir.z); 
        float band = smoothstep(0.40, 0.0, abs(lat)); // Wide fade for the main band
        float core = smoothstep(0.15, 0.0, abs(lat)); // Tight inner core

        // Noise layers for deep space clouds
        float t = uTime * 0.002;
        float n1 = fbm3(dir * 2.5 + vec3(t));
        float n2 = fbm3(dir * 5.0 - vec3(t * 0.7));
        float n3 = fbm3(dir * 12.0); // Fine dust
        
        // Shape of the nebula clouds
        float cloud = (n1 * 0.5 + n2 * 0.35 + n3 * 0.15);
        
        // Combine band mask with noise
        float structure = cloud * (band * 0.6 + 0.4); // 40% everywhere, 100% at band
        float denseCore = cloud * core * n2;
        
        structure = smoothstep(0.25, 0.70, structure);

        // Color palette (Deep rich purples, soft magenta, fiery core)
        vec3 spaceDark = vec3(0.01, 0.01, 0.02);     // Void
        vec3 deepPurple= vec3(0.08, 0.03, 0.15);     // Faint cosmic dust
        vec3 nebulaPink= vec3(0.18, 0.05, 0.15);     // Colored gas
        vec3 warmOrange= vec3(0.25, 0.10, 0.05);     // Core dust
        
        vec3 color = spaceDark;
        color = mix(color, deepPurple, smoothstep(0.0, 0.3, structure));
        color = mix(color, nebulaPink, smoothstep(0.25, 0.6, structure));
        color = mix(color, warmOrange, smoothstep(0.5, 1.0, structure));
        
        // Add extreme center brightness at the galactic core
        color += vec3(0.15, 0.05, 0.02) * denseCore;

        FragColor = vec4(color, 1.0);
        return;
    }


    // ── SUN SURFACE ──
    // Reference: bright yellow-white hot spots, near-black spots with red hint,
    // dark orange granules, bright yellow-white limb boundary
    if (GLOW) {
        float NdotV = max(dot(N, V), 0.0);

        // Seamless 3D coordinates based on normal vector (zero blockiness or seams!)
        vec3 p = N * 2.5;
        float t = uTime * 0.012;

        float n1 = fbm(p * 2.0 + vec3(t));
        float n2 = fbm(p * 4.0 - vec3(t*0.7, 0, t*0.7) + 2.7);
        float n3 = fbm(p * 1.2 + vec3(t*0.5, t*0.5, -t*0.4));
        float n4 = fbm(p * 7.0 + vec3(0, t*0.3, 0));

        // Solar granulation (dark orange network)
        float granule = n1 * 0.5 + n2 * 0.3 + 0.35;
        granule = smoothstep(0.25, 0.85, granule);

        // Sunspot mask — near-black spots with red undertone (reduced abundance)
        float spotMask = smoothstep(0.18, 0.28, n3);

        // Bright yellow-white hot spots (plages/flares)
        float hotSpot = pow(max(n4 - 0.45, 0.0) * 2.5, 2.5);
        float hotSpot2 = pow(max(n1 - 0.55, 0.0) * 3.0, 2.0);

        // Color palette (matching reference image)
        vec3 brightWhite = vec3(1.0, 0.97, 0.85);  // yellow-white hot spots
        vec3 midOrange   = vec3(1.0, 0.65, 0.15);  // mid surface orange
        vec3 darkOrange  = vec3(0.75, 0.35, 0.05);  // dark orange granules
        vec3 spotBlack   = vec3(0.12, 0.04, 0.02);  // near-black spots (red hint)
        vec3 limbBright  = vec3(1.0, 0.95, 0.70);   // bright yellow-white limb edge

        // Base: orange surface
        float limb = pow(NdotV, 0.5);
        vec3 col = mix(darkOrange, midOrange, limb);

        // Granulation darkens areas (dark orange network)
        col = mix(darkOrange * 0.6, col, granule);

        // Sunspots — nearly black with red hint
        col = mix(spotBlack, col, spotMask);

        // Bright hot spots — yellow-white flares
        col = mix(col, brightWhite * 1.5, hotSpot * 0.5);
        col = mix(col, brightWhite * 1.3, hotSpot2 * 0.3);

        // Brightness falloff
        col *= limb * 1.5 + 0.15;

        // Bright limb edge (yellow-white boundary — reference shows bright edge!)
        float edge = pow(1.0 - NdotV, 1.5);
        float edgeBright = smoothstep(0.3, 0.8, edge);
        col = mix(col, limbBright * 1.2, edgeBright * 0.6);

        // Thin chromosphere ring
        float chromo = pow(1.0 - NdotV, 8.0) * 2.5;
        col += vec3(1.0, 0.70, 0.20) * chromo;

        FragColor = vec4(col, 1.0);
        return;
    }

    // ── PLANETS (Blinn-Phong) ──
    float diff = max(dot(N,L), 0.0);
    float spec = diff > 0.0 ? pow(max(dot(N,H),0.0), 64.0) : 0.0;
    float rim = pow(1.0 - max(dot(N,V),0.0), 3.0) * 0.35;
    vec3 amb = objectColor.rgb * ambient;
    vec3 dif = objectColor.rgb * diff;
    vec3 spe = vec3(1.0) * spec * 0.4;
    vec3 rimC = objectColor.rgb * rim;
    float term = smoothstep(-0.05, 0.15, diff);
    vec3 night = objectColor.rgb * 0.03;
    vec3 lit = amb + dif*term + spe*term + rimC;
    vec3 fin = mix(night + amb + rimC, lit, term);
    FragColor = vec4(fin, objectColor.a);
})glsl";
