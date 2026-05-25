#version 430 core

const float PI = 3.14159265359;

uniform sampler2D albedo_atlas;
uniform sampler2D normal_atlas;
uniform sampler2D specular_atlas;

uniform vec3 camera_pos;
uniform vec3 sun_direction;
uniform vec3 sun_color;
uniform sampler2D sun_shadowmap;
uniform mat4 sun_VP;

// TODO: lights uniforms
const int nb_lights = 2;
const vec3 lightPositions[nb_lights] = vec3[](vec3(23.5, 6., 29.5), vec3(20.5, 6., 29.5));
const vec3 lightColors[nb_lights] = vec3[](vec3(1., 1., 0.), vec3(1., 0., 0.));
// const vec3 lightShadowMaps[nb_lights] = sampler2D[](vec3(1., 1., 0.), vec3(1., 0., 0.));
// const vec3 lightViewProjection[nb_lights] = vec3[](vec3(1., 1., 0.), vec3(1., 0., 0.));

in vec3 f_worldpos;
in vec3 f_normal;
in vec2 f_uv;
in mat3 f_TBN;

out vec4 out_color;

const int pcf_filter_size = 1;
const float one_over_nshadows = 1. / ((2 * pcf_filter_size + 1) * (2 * pcf_filter_size + 1));
float getShadowFactor(vec4 _fragpos_light_space, sampler2D _shadowmap) {
  vec3 proj_coords = _fragpos_light_space.xyz / _fragpos_light_space.w;
  proj_coords = proj_coords * 0.5f + 0.5f;
  if (proj_coords.x < 0.f || 1.f < proj_coords.x ||
      proj_coords.y < 0.f || 1.f < proj_coords.y)
    return 0.f;
  float current_depth = proj_coords.z;

  float shadow = 1.f;
  vec2 texel_size = 1.0 / textureSize(_shadowmap, 0);
  for (int y = -pcf_filter_size; y <= pcf_filter_size; y++) {
    for (int x = -pcf_filter_size; x <= pcf_filter_size; x++) {
      vec2 offset = vec2(x, y) * texel_size;
      float closest_depth = texture(_shadowmap, proj_coords.xy + offset).r;
      shadow -= current_depth > closest_depth ? one_over_nshadows : 0.;
    }
  }

  return shadow;
}

// PBR

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
  // ALBEDO DATA
  vec4 f_albedo = texture(albedo_atlas, f_uv).rgba;
  vec3 albedo = pow(f_albedo.xyz, vec3(2.2));
  float transparency = f_albedo.a;
  if (transparency == 0.)
    discard;
  out_color = vec4(0., 0., 0., transparency);

  // PBR DATA
  vec4 pbr = texture(specular_atlas, f_uv).rgba;
  float roughness = 1. - pbr.r;
  float metallic = pbr.g;
  vec3 F0 = vec3(pbr.b);
  float ao = 1.;
  float emission = 0.;
  F0 = mix(F0, albedo, metallic); // vec3(0.04);

  // NORMAL DATA
  vec4 f_normal_map = texture(normal_atlas, f_uv).rgba;
  vec3 tangent_normal = f_normal_map.xyz * 2.0 - 1.0;
  vec3 mapped_normal = normalize(f_TBN * tangent_normal);
  vec3 N = normalize(mix(f_normal, mapped_normal, f_normal_map.a));
  vec3 V = normalize(camera_pos - f_worldpos);

  // reflectance equation
  vec3 Lo = vec3(0.);
  for (int i = 0; i <= nb_lights; i++) {
    // calculate the frag illumination
    float shadow = 1.f;
    if (i == nb_lights) {
      vec4 fragPosInLightSpace = sun_VP * vec4(f_worldpos, 1.);
      shadow = getShadowFactor(fragPosInLightSpace, sun_shadowmap);
    }
    if (shadow == 0.f)
      continue;

    // calculate per-light radiance
    vec3 L = i == nb_lights ? sun_direction : lightPositions[i] - f_worldpos;
    float distance = length(L);
    L /= distance;

    vec3 H = normalize(V + L);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = (i == nb_lights ? sun_color : lightColors[i]) * attenuation;

    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 kS = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    kS = 1.0 - kD; // car on doit avoir kD + kS = 1

    vec3 numerator = NDF * G * kS;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadow;
  }

  vec3 ambient_light = vec3(0.03) * albedo * ao;
  vec3 emited_light = albedo * emission;
  out_color.xyz = ambient_light + emited_light + Lo;

  out_color.xyz = out_color.xyz / (out_color.xyz + vec3(1.0));
  out_color.xyz = pow(out_color.xyz, vec3(1.0 / 2.2));
}