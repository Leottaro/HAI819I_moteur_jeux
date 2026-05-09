#version 430 core

const float PI = 3.14159265359;

uniform sampler2D albedo_atlas;
uniform sampler2D normal_atlas;
uniform sampler2D specular_atlas;
uniform vec3 camera_pos;

uniform vec2 sun_pos;
uniform vec3 sun_color;

in vec3 f_worldpos;
in vec3 f_normal;
in vec2 f_uv;
in mat3 f_TBN;

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

void main() {
  vec4 f_albedo = texture(albedo_atlas, f_uv).rgba;
  vec4 f_normal_map = texture(normal_atlas, f_uv).rgba;
  vec3 albedo = pow(f_albedo.xyz, vec3(2.2));
  float transparency = f_albedo.a;
  if(transparency == 0.)
    discard;
  out_color = vec4(0., 0., 0., transparency);

  vec4 pbr = texture(specular_atlas, f_uv).rgba;
  float roughness = 1. - pbr.r;
  float metallic = pbr.g;
  vec3 F0 = vec3(pbr.b);
  float ao = 1.;
  float emission = 0.;

  vec3 tangent_normal = f_normal_map.xyz * 2.0 - 1.0;
  vec3 mapped_normal = normalize(f_TBN * tangent_normal);

  vec3 N = normalize(mix(f_normal, mapped_normal, f_normal_map.a));
  vec3 V = normalize(camera_pos - f_worldpos);

  F0 = mix(F0, albedo, metallic);

  // reflectance equation
  vec3 Lo = vec3(0.);
    // calculate per-light radiance
  vec3 L = vec3(sin(sun_pos.x), cos(sun_pos.x) * sin(sun_pos.y), cos(sun_pos.x) * cos(sun_pos.y));
  vec3 H = normalize(V + L);
  vec3 radiance = sun_color;

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

  vec3 ambient_light = vec3(0.01) * albedo * ao;
  vec3 emited_light = albedo * emission;
  out_color.xyz = ambient_light + emited_light + Lo;

  out_color.xyz = out_color.xyz / (out_color.xyz + vec3(1.0));
  out_color.xyz = pow(out_color.xyz, vec3(1.0 / 2.2));
}