#version 430 core

const float PI = 3.14159265359;

uniform sampler2D albedo_atlas;
uniform sampler2D normal_atlas;
uniform sampler2D specular_atlas;
uniform vec3 camera_pos;

uniform vec2 sun_pos;

in vec3 f_worldpos;
in vec3 f_normal;
in vec2 f_uv;

out vec4 out_color;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float num = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float num = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

vec3 bakeLight(vec3 lightPos, vec3 lightCol, vec3 N, vec3 V, float roughness, float metallic, vec3 F0, vec3 albedo) {
  // calculate per-light radiance
  vec3 L = lightPos;
  vec3 H = normalize(V + L);
  float distance = length(lightPos - f_worldpos);
  float attenuation = 1.0 / (distance * distance);
  vec3 radiance = lightCol * attenuation;

  // cook-torrance brdf
  float NDF = DistributionGGX(N, H, roughness);
  float G = GeometrySmith(N, V, L, roughness);
  vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  vec3 numerator = NDF * G * F;
  float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
  vec3 specular = numerator / denominator;

  // add to outgoing radiance Lo
  float NdotL = max(dot(N, L), 0.0);
  return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
  vec4 f_albedo = texture(albedo_atlas, f_uv).rgba;
  vec4 f_normal_map = texture(normal_atlas, f_uv).rgba;
  vec3 albedo = pow(f_albedo.xyz, vec3(2.2));
  float transparency = f_albedo.a;
  if (transparency == 0.)
    discard;
  out_color = vec4(0., 0., 0., transparency);

  // TODO: use normal map

  vec4 pbr = texture(specular_atlas, f_uv).rgba;
  float roughness = 1. - pbr.r;
  float metallic = pbr.g;
  vec3 F0 = vec3(pbr.b);
  float ao = 1.;
  float emission = 0.;

  // TODO: lights uniforms
  int nbLights = 3;
  vec3 lightPositions[3] = vec3[](vec3(0., 1., 1.) + f_worldpos, vec3(16., 10., 16.), vec3(24., 10., 12.));
  vec3 lightColors[3] = vec3[](vec3(1.), vec3(1.), vec3(1., 0., 0.));

  float normal_ratio = 255. - f_normal_map.w;

  vec3 N = normalize(f_normal_map.xyz * f_normal_map.w + normal_ratio * f_normal);
  vec3 V = normalize(camera_pos - f_worldpos);

  // vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // reflectance equation
  vec3 Lo = vec3(0.);
  for (int i = 0; i < 1; i++) {
    // calculate per-light radiance
    vec3 L = vec3(sin(sun_pos.x), cos(sun_pos.x) * sin(sun_pos.y), cos(sun_pos.x) * cos(sun_pos.y));
    vec3 H = normalize(V + L);
    vec3 radiance = vec3(1.f);

    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }
  // vec3 sun_vec = vec3(cos(sun_pos.x) * cos(sun_pos.y), cos(sun_pos.x) * sin(sun_pos.y), sin(sun_pos.x));
  // Lo += bakeLight(sun_vec, vec3(1.f), N, V, roughness, metallic, F0, albedo);
  // for (int i = 0; i < 3; i++) {
  //   // calculate per-light radiance
  //   vec3 L = normalize(lightPositions[i] - f_worldpos);
  //   vec3 H = normalize(V + L);
  //   float distance = length(lightPositions[i] - f_worldpos);
  //   float attenuation = 1.0 / (distance * distance);
  //   vec3 radiance = lightColors[i] * attenuation;

  //   // cook-torrance brdf
  //   float NDF = DistributionGGX(N, H, roughness);
  //   float G = GeometrySmith(N, V, L, roughness);
  //   vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

  //   vec3 kS = F;
  //   vec3 kD = vec3(1.0) - kS;
  //   kD *= 1.0 - metallic;

  //   vec3 numerator = NDF * G * F;
  //   float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
  //   vec3 specular = numerator / denominator;

  //   // add to outgoing radiance Lo
  //   float NdotL = max(dot(N, L), 0.0);
  //   Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  // }

  vec3 ambient_light = vec3(0.01) * albedo * ao;
  vec3 emited_light = albedo * emission;
  vec3 color = ambient_light + emited_light + Lo;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  out_color = vec4(color, 1.0);
}