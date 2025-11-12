#include "graphics.h"

#define VARIFYHEAP(ptr, str, type)                                             \
  do {                                                                         \
    if (!(ptr)) {                                                              \
      printf("Heap allocation error: %s\n", str);                              \
      return type;                                                             \
    }                                                                          \
  } while (0)

static int min(int a, int b) { return a < b ? a : b; }
static int max(int a, int b) { return a > b ? a : b; }

static void set_pixel(SDL_Surface *s, uint16_t x, uint16_t y, uint8_t r,
                      uint8_t g, uint8_t b) {
  if (x >= (uint16_t)s->w || y >= (uint16_t)s->h)
    return;
  uint32_t v = SDL_MapRGB(s->format, r, g, b);
  ((uint32_t *)s->pixels)[y * (s->pitch / 4) + x] = v;
}

static void set_pixel_zbuffered(SDL_Surface *s, uint32_t *zbuffer, uint16_t x,
                                uint16_t y, uint8_t r, uint8_t g, uint8_t b,
                                float z) {
  if (x >= (uint16_t)s->w || y >= (uint16_t)s->h)
    return;
  uint32_t pixel_index_pixels = y * (s->pitch / 4) + x;
  uint32_t pixel_index_z = y * s->w + x;

    float z_clamped = fmaxf(0.0f, fminf(1.0f, z));
    uint32_t z_int = (uint32_t)(z_clamped * 4294967295.0f);

    uint32_t stored = zbuffer[pixel_index_z];
    if (z_int < stored - 1u) {
      zbuffer[pixel_index_z] = z_int;
      uint32_t v = SDL_MapRGB(s->format, r, g, b);
      ((uint32_t *)s->pixels)[pixel_index_pixels] = v;
    }
}

static bbox2i calculate_bbox2i_from_tri(vec2i v1, vec2i v2, vec2i v3) {
  bbox2i b = {.min = {v1[0], v1[1]}, .max = {v1[0], v1[1]}};
  if (v2[0] < b.min[0])
    b.min[0] = v2[0];
  if (v2[1] < b.min[1])
    b.min[1] = v2[1];
  if (v2[0] > b.max[0])
    b.max[0] = v2[0];
  if (v2[1] > b.max[1])
    b.max[1] = v2[1];
  if (v3[0] < b.min[0])
    b.min[0] = v3[0];
  if (v3[1] < b.min[1])
    b.min[1] = v3[1];
  if (v3[0] > b.max[0])
    b.max[0] = v3[0];
  if (v3[1] > b.max[1])
    b.max[1] = v3[1];
  return b;
}

static inline void mat4_transpose(mat4 out, const mat4 m) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      out[i][j] = m[j][i];
    }
}

static inline void mat4_identity(mat4 m) {
  memset(m, 0, sizeof(mat4));
  m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

static inline void mat4_inverse(mat4 out, const mat4 m) {
  float det = m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
              m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
              m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
  if (det == 0) {
    memset(out, 0, sizeof(mat4));
    return;
  }
  float inv_det = 1.0f / det;
  out[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * inv_det;
  out[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * inv_det;
  out[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * inv_det;
  out[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * inv_det;
  out[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * inv_det;
  out[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * inv_det;
  out[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * inv_det;
  out[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * inv_det;
  out[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * inv_det;
  out[3][0] =
      -(out[0][0] * m[3][0] + out[1][0] * m[3][1] + out[2][0] * m[3][2]);
  out[3][1] =
      -(out[0][1] * m[3][0] + out[1][1] * m[3][1] + out[2][1] * m[3][2]);
  out[3][2] =
      -(out[0][2] * m[3][0] + out[1][2] * m[3][1] + out[2][2] * m[3][2]);
  out[3][3] = 1.0f;
}

static inline void mat4_mul(mat4 out, const mat4 a, const mat4 b) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      out[i][j] = a[0][j] * b[i][0] + a[1][j] * b[i][1] + a[2][j] * b[i][2] +
                  a[3][j] * b[i][3];
    }
}

static inline void mat4_vec3_mul(vec3 out, const mat4 m, const vec3 v) {
  float x = v[0], y = v[1], z = v[2];
  out[0] = x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0];
  out[1] = x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1];
  out[2] = x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2];
}

static void mat4_transform_clip(vec4 out, vec3 v, mat4 m) {
  float x = v[0], y = v[1], z = v[2];
  out[0] = x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0];
  out[1] = x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1];
  out[2] = x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2];
  out[3] = x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3];
}

static void clip_polygon_component(clip_vertex *input, int in_count,
                                   clip_vertex *output, int *out_count,
                                   int component, int positive) {
  *out_count = 0;

  if (in_count == 0)
    return;

  clip_vertex prev = input[in_count - 1];

  for (int i = 0; i < in_count; ++i) {
    clip_vertex curr = input[i];

    float prev_val =
        component == 0 ? prev.p[0] : (component == 1 ? prev.p[1] : prev.p[2]);
    float curr_val =
        component == 0 ? curr.p[0] : (component == 1 ? curr.p[1] : curr.p[2]);

    float prev_boundary = positive ? (prev.w - prev_val) : (prev.w + prev_val);
    float curr_boundary = positive ? (curr.w - curr_val) : (curr.w + curr_val);

    int prev_inside = (prev_boundary >= 0);
    int curr_inside = (curr_boundary >= 0);

    if (curr_inside) {
      if (!prev_inside) {
        float t = prev_boundary / (prev_boundary - curr_boundary);
        clip_vertex new_vert;
        new_vert.p[0] = prev.p[0] + t * (curr.p[0] - prev.p[0]);
        new_vert.p[1] = prev.p[1] + t * (curr.p[1] - prev.p[1]);
        new_vert.p[2] = prev.p[2] + t * (curr.p[2] - prev.p[2]);
        new_vert.w = prev.w + t * (curr.w - prev.w);
        output[(*out_count)++] = new_vert;
      }
      output[(*out_count)++] = curr;
    } else if (prev_inside) {
      float t = prev_boundary / (prev_boundary - curr_boundary);
      clip_vertex new_vert;
      new_vert.p[0] = prev.p[0] + t * (curr.p[0] - prev.p[0]);
      new_vert.p[1] = prev.p[1] + t * (curr.p[1] - prev.p[1]);
      new_vert.p[2] = prev.p[2] + t * (curr.p[2] - prev.p[2]);
      new_vert.w = prev.w + t * (curr.w - prev.w);
      output[(*out_count)++] = new_vert;
    }

    prev = curr;
  }
}

void update_view_matrix(mat4 *mat, camera c) {
  float cx = cosf(c.rotation[0] * 3.14159265f / 180.0f);
  float sx = sinf(c.rotation[0] * 3.14159265f / 180.0f);
  float cy = cosf(c.rotation[1] * 3.14159265f / 180.0f);
  float sy = sinf(c.rotation[1] * 3.14159265f / 180.0f);

  vec3 forward = {-sy * cx, -sx, -cy * cx};
  vec3 up = {0.0f, 1.0f, 0.0f};
  float len = sqrtf(forward[0] * forward[0] + forward[1] * forward[1] +
                    forward[2] * forward[2]);
  if (len > 0.0f) {
    forward[0] /= len;
    forward[1] /= len;
    forward[2] /= len;
  }

  vec3 right = {forward[1] * up[2] - forward[2] * up[1],
                forward[2] * up[0] - forward[0] * up[2],
                forward[0] * up[1] - forward[1] * up[0]};

  up[0] = right[1] * forward[2] - right[2] * forward[1];
  up[1] = right[2] * forward[0] - right[0] * forward[2];
  up[2] = right[0] * forward[1] - right[1] * forward[0];

  mat4_identity(*mat);
  (*mat)[0][0] = right[0];
  (*mat)[0][1] = right[1];
  (*mat)[0][2] = right[2];
  (*mat)[1][0] = up[0];
  (*mat)[1][1] = up[1];
  (*mat)[1][2] = up[2];
  (*mat)[2][0] = -forward[0];
  (*mat)[2][1] = -forward[1];
  (*mat)[2][2] = -forward[2];

  (*mat)[3][0] = -(c.position[0] * right[0] + c.position[1] * up[0] +
                   c.position[2] * (-forward[0]));
  (*mat)[3][1] = -(c.position[0] * right[1] + c.position[1] * up[1] +
                   c.position[2] * (-forward[1]));
  (*mat)[3][2] = -(c.position[0] * right[2] + c.position[1] * up[2] +
                   c.position[2] * (-forward[2]));
}

void update_model_matrix(mat4 *mat, vec3 pos, vec3 pivot, vec3 rot_deg) {
  const float to_rad = 3.14159265f / 180.0f;
  float ax = rot_deg[0] * to_rad, ay = rot_deg[1] * to_rad,
        az = rot_deg[2] * to_rad;

  float cx = cosf(ax), sx = sinf(ax);
  float cy = cosf(ay), sy = sinf(ay);
  float cz = cosf(az), sz = sinf(az);

  mat4 R;
  mat4_identity(R);

  R[0][0] = cy * cz;
  R[0][1] = cx * sz + sx * sy * cz;
  R[0][2] = sx * sz - cx * sy * cz;

  R[1][0] = -cy * sz;
  R[1][1] = cx * cz - sx * sy * sz;
  R[1][2] = sx * cz + cx * sy * sz;

  R[2][0] = sy;
  R[2][1] = -sx * cy;
  R[2][2] = cx * cy;

  mat4 T_pivot, T_minus_pivot;
  mat4_identity(T_pivot);
  mat4_identity(T_minus_pivot);

  T_pivot[3][0] = pivot[0];
  T_pivot[3][1] = pivot[1];
  T_pivot[3][2] = pivot[2];

  T_minus_pivot[3][0] = -pivot[0];
  T_minus_pivot[3][1] = -pivot[1];
  T_minus_pivot[3][2] = -pivot[2];

  mat4 R_prime;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      R_prime[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k)
        R_prime[i][j] += R[i][k] * T_minus_pivot[k][j];
    }

  mat4 M;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      M[i][j] = 0.0f;
      for (int k = 0; k < 4; ++k)
        M[i][j] += T_pivot[i][k] * R_prime[k][j];
    }

  M[3][0] += pos[0];
  M[3][1] += pos[1];
  M[3][2] += pos[2];

  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      (*mat)[i][j] = M[i][j];
}

void update_projection_matrix(mat4 *mat, camera c, uint16_t width,
                              uint16_t height) {
  float f = 1.0f / tanf(c.fovy * 0.5f * 3.14159265f / 180.0f);
  float a = (float)height / (float)width;
  float q = c.far / (c.far - c.near);

  memset(mat, 0, sizeof(mat4));
  (*mat)[0][0] = f * a;
  (*mat)[1][1] = -f;
  (*mat)[2][2] = q;
  (*mat)[2][3] = 1.0f;
  (*mat)[3][2] = -c.near * q;
  (*mat)[3][3] = 0.0f;
}

void draw_line_to_backbuffer(SDL_Surface *surface, uint8_t r, uint8_t g,
                             uint8_t b, uint16_t x1, uint16_t y1, uint16_t x2,
                             uint16_t y2) {
  VARIFYHEAP(surface, "draw_line_to_backbuffer()", );
  if (!surface->pixels)
    return;

  int dx = abs((int)x2 - (int)x1);
  int dy = abs((int)y2 - (int)y1);
  int sx = x1 < x2 ? 1 : -1;
  int sy = y1 < y2 ? 1 : -1;
  int err = dx - dy;
  uint32_t pixel = SDL_MapRGB(surface->format, r, g, b);
  uint32_t *pixels = (uint32_t *)surface->pixels;
  int w = surface->w;

  while (1) {
    if (x1 < surface->clip_rect.w && y1 < surface->clip_rect.h &&
        x1 >= surface->clip_rect.x && y1 >= surface->clip_rect.y)
      pixels[w * y1 + x1] = pixel;
    if (x1 == x2 && y1 == y2)
      break;
    int e2 = err * 2;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

void draw_wireframe_tri_to_backbuffer(SDL_Surface *surface, vec2i v1, vec2i v2,
                                      vec2i v3, uint8_t r, uint8_t g, uint8_t b,
                                      int debug) {
  if (debug) {
    draw_line_to_backbuffer(surface, 255, 0, 0, v1[0], v1[1], v2[0], v2[1]);
    draw_line_to_backbuffer(surface, 0, 255, 0, v2[0], v2[1], v3[0], v3[1]);
    draw_line_to_backbuffer(surface, 0, 0, 255, v3[0], v3[1], v1[0], v1[1]);
    set_pixel(surface, v1[0], v1[1], 255, 255, 255);
    set_pixel(surface, v2[0], v2[1], 255, 255, 255);
    set_pixel(surface, v3[0], v3[1], 255, 255, 255);
    return;
  }
  draw_line_to_backbuffer(surface, r, g, b, v1[0], v1[1], v2[0], v2[1]);
  draw_line_to_backbuffer(surface, r, g, b, v2[0], v2[1], v3[0], v3[1]);
  draw_line_to_backbuffer(surface, r, g, b, v3[0], v3[1], v1[0], v1[1]);
}

void draw_tri_to_backbuffer(SDL_Surface *surface, vec2i v1, vec2i v2, vec2i v3,
                            uint8_t r, uint8_t g, uint8_t b, int debug) {
  if (debug) {
    bbox2i bb = calculate_bbox2i_from_tri(v1, v2, v3);
    uint16_t sx = max(0, bb.min[0]), ex = min(surface->w - 1, bb.max[0]);
    uint16_t sy = max(0, bb.min[1]), ey = min(surface->h - 1, bb.max[1]);

    float x0 = (float)v1[0], y0 = (float)v1[1];
    float x1f = (float)v2[0], y1f = (float)v2[1];
    float x2f = (float)v3[0], y2f = (float)v3[1];

    float area = (x1f - x0) * (y2f - y0) - (y1f - y0) * (x2f - x0);
    if (fabsf(area) < 1e-8f)
      return;

    float A0 = y1f - y2f;
    float B0 = x2f - x1f;
    float C0 = x1f * y2f - x2f * y1f;

    float A1 = y2f - y0;
    float B1 = x0 - x2f;
    float C1 = x2f * y0 - x0 * y2f;

    float A2 = y0 - y1f;
    float B2 = x1f - x0;
    float C2 = x0 * y1f - x1f * y0;

    float start_px = (float)sx + 0.5f;
    float start_py = (float)sy + 0.5f;

    float e0_row = A0 * start_px + B0 * start_py + C0;
    float e1_row = A1 * start_px + B1 * start_py + C1;
    float e2_row = A2 * start_px + B2 * start_py + C2;

    float step_x_e0 = A0;
    float step_x_e1 = A1;
    float step_x_e2 = A2;

    float eps = -1e-6f;

    for (uint16_t y = sy; y <= ey; ++y) {
      float e0 = e0_row;
      float e1 = e1_row;
      float e2 = e2_row;
      for (uint16_t x = sx; x <= ex; ++x) {
        if (e0 >= eps && e1 >= eps && e2 >= eps) {
          if (debug) {
            float inv_area = 1.0f / area;
            float u = e0 * inv_area;
            float v = e1 * inv_area;
            float w = 1.0f - u - v;
            uint8_t rc = (uint8_t)fmaxf(0.0f, fminf(255.0f, u * 255.0f));
            uint8_t gc = (uint8_t)fmaxf(0.0f, fminf(255.0f, v * 255.0f));
            uint8_t bc = (uint8_t)fmaxf(0.0f, fminf(255.0f, w * 255.0f));
            set_pixel(surface, x, y, rc, gc, bc);
          } else {
            set_pixel(surface, x, y, r, g, b);
          }
        }
        e0 += step_x_e0;
        e1 += step_x_e1;
        e2 += step_x_e2;
      }
      e0_row += B0;
      e1_row += B1;
      e2_row += B2;
    }
    return;
  }

  bbox2i bb = calculate_bbox2i_from_tri(v1, v2, v3);
  uint16_t sx = max(0, bb.min[0]), ex = min(surface->w - 1, bb.max[0]);
  uint16_t sy = max(0, bb.min[1]), ey = min(surface->h - 1, bb.max[1]);
  float det =
      (v2[1] - v3[1]) * (v1[0] - v3[0]) + (v3[0] - v2[0]) * (v1[1] - v3[1]);
  if (fabsf(det) < 1e-8f)
    return;
  float inv = 1.0f / det;
  for (uint16_t y = sy; y <= ey; ++y)
    for (uint16_t x = sx; x <= ex; ++x) {
      /* sample at pixel center to avoid edge/corner artifacts */
      float px = (float)x + 0.5f;
      float py = (float)y + 0.5f;
      float u =
          ((v2[1] - v3[1]) * (px - v3[0]) + (v3[0] - v2[0]) * (py - v3[1])) *
          inv;
      float v =
          ((v3[1] - v1[1]) * (px - v3[0]) + (v1[0] - v3[0]) * (py - v3[1])) *
          inv;
      float w = 1.0f - u - v;
      if (u >= -1e-6f && v >= -1e-6f && w >= -1e-6f)
        set_pixel(surface, x, y, r, g, b);
    }
}

static void draw_tri_to_backbuffer_zbuffered(
    SDL_Surface *surface, uint32_t *zbuffer, vec2i v1, vec2i v2, vec2i v3,
    uint8_t r, uint8_t g, uint8_t b, float z1_over_w, float oow1,
    float z2_over_w, float oow2, float z3_over_w, float oow3) {
  bbox2i bb = calculate_bbox2i_from_tri(v1, v2, v3);
  uint16_t sx = max(0, bb.min[0]), ex = min(surface->w - 1, bb.max[0]);
  uint16_t sy = max(0, bb.min[1]), ey = min(surface->h - 1, bb.max[1]);
  float det =
      (v2[1] - v3[1]) * (v1[0] - v3[0]) + (v3[0] - v2[0]) * (v1[1] - v3[1]);
  if (fabsf(det) < 1e-8f)
    return;
  float inv = 1.0f / det;
  for (uint16_t y = sy; y <= ey; ++y)
    for (uint16_t x = sx; x <= ex; ++x) {
      float px = (float)x + 0.5f;
      float py = (float)y + 0.5f;
      float u =
          ((v2[1] - v3[1]) * (px - v3[0]) + (v3[0] - v2[0]) * (py - v3[1])) *
          inv;
      float v =
          ((v3[1] - v1[1]) * (px - v3[0]) + (v1[0] - v3[0]) * (py - v3[1])) *
          inv;
      float w = 1.0f - u - v;
      if (u >= -1e-6f && v >= -1e-6f && w >= -1e-6f) {
        float interp_oow = u * oow1 + v * oow2 + w * oow3;
        if (interp_oow <= 1e-8f)
          continue;
        float interp_zow = u * z1_over_w + v * z2_over_w + w * z3_over_w;
        float z = interp_zow / interp_oow;
        set_pixel_zbuffered(surface, zbuffer, x, y, r, g, b, z);
      }
    }
}

void draw_tri3d_to_backbuffer(SDL_Surface *surface, camera c, vec3 v1, vec3 v2,
                              vec3 v3, uint8_t r, uint8_t g, uint8_t b,
                              vec3 pos, vec3 rot, vec3 pivot, int debug) {

  mat4 model, view, proj, mv, mvp;
  update_model_matrix(&model, pos, pivot, rot);
  update_view_matrix(&view, c);
  update_projection_matrix(&proj, c, surface->w, surface->h);
  mat4_mul(mv, view, model);
  mat4_mul(mvp, proj, mv);

  vec4 clip1, clip2, clip3;
  mat4_transform_clip(clip1, v1, mvp);
  mat4_transform_clip(clip2, v2, mvp);
  mat4_transform_clip(clip3, v3, mvp);

  vec3 normal;
  vec3 edge1, edge2;

  edge1[0] = v2[0] - v1[0];
  edge1[1] = v2[1] - v1[1];
  edge1[2] = v2[2] - v1[2];
  edge2[0] = v3[0] - v1[0];
  edge2[1] = v3[1] - v1[1];
  edge2[2] = v3[2] - v1[2];

  normal[0] = edge1[1] * edge2[2] - edge1[2] * edge2[1];
  normal[1] = edge1[2] * edge2[0] - edge1[0] * edge2[2];
  normal[2] = edge1[0] * edge2[1] - edge1[1] * edge2[0];

  float l = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] +
                  normal[2] * normal[2]);

  normal[0] /= l;
  normal[1] /= l;
  normal[2] /= l;

  mat4 model_inv;
  mat4_inverse(model_inv, model);
  mat4 normal_matrix;
  mat4_transpose(normal_matrix, model_inv);

  vec3 normal_world;
  mat4_vec3_mul(normal_world, normal_matrix, normal);

  if (clip1[3] <= 0 || clip2[3] <= 0 || clip3[3] <= 0)
    return;

  clip_vertex input_verts[6], output_verts[6];
  int count = 3;
  input_verts[0] =
      (clip_vertex){.p = {clip1[0], clip1[1], clip1[2]}, .w = clip1[3]};
  input_verts[1] =
      (clip_vertex){.p = {clip2[0], clip2[1], clip2[2]}, .w = clip2[3]};
  input_verts[2] =
      (clip_vertex){.p = {clip3[0], clip3[1], clip3[2]}, .w = clip3[3]};

  clip_vertex temp[6];
  int temp_count;

  clip_polygon_component(input_verts, count, output_verts, &temp_count, 0, 0);
  if (temp_count < 3)
    return;
  clip_polygon_component(output_verts, temp_count, temp, &count, 0, 1);
  if (count < 3)
    return;
  clip_polygon_component(temp, count, output_verts, &temp_count, 1, 0);
  if (temp_count < 3)
    return;
  clip_polygon_component(output_verts, temp_count, temp, &count, 1, 1);
  if (count < 3)
    return;
  clip_polygon_component(temp, count, output_verts, &temp_count, 2, 1);
  if (temp_count < 3)
    return;

  clip_vertex verts[6];
  count = temp_count;
  for (int i = 0; i < count; ++i)
    verts[i] = output_verts[i];

  for (int i = 0; i < count; ++i) {
    float w = verts[i].w;
    if (w > 0.0001f) {
      verts[i].p[0] /= w;
      verts[i].p[1] /= w;
      verts[i].p[2] /= w;
    } else {
      verts[i].p[0] = verts[i].p[1] = verts[i].p[2] = 0.0f;
    }
  }

  vec2i screen[6];
  for (int i = 0; i < count; ++i) {
    float ndc_x = verts[i].p[0];
    float ndc_y = verts[i].p[1];

    float x = (ndc_x * 0.5f + 0.5f) * surface->w;
    float y = (1.0f - (ndc_y * 0.5f + 0.5f)) * surface->h;

    int ix = (int)x;
    int iy = (int)y;

    ix = ix < 0 ? 0 : (ix >= surface->w ? surface->w - 1 : ix);
    iy = iy < 0 ? 0 : (iy >= surface->h ? surface->h - 1 : iy);

    screen[i][0] = ix;
    screen[i][1] = iy;
  }

  vec3 normal_rgb;
  float r_f = (normal_world[0] + 1.0f) * 0.5f * 255.0f;
  float g_f = (normal_world[1] + 1.0f) * 0.5f * 255.0f;
  float b_f = (normal_world[2] + 1.0f) * 0.5f * 255.0f;

  normal_rgb[0] = r_f < 0.0f ? 0 : (r_f > 255.0f ? 255 : (uint8_t)r_f);
  normal_rgb[1] = g_f < 0.0f ? 0 : (g_f > 255.0f ? 255 : (uint8_t)g_f);
  normal_rgb[2] = b_f < 0.0f ? 0 : (b_f > 255.0f ? 255 : (uint8_t)b_f);

  for (int i = 1; i < count - 1; ++i) {
    int ax = screen[i][0] - screen[0][0];
    int ay = screen[i][1] - screen[0][1];
    int bx = screen[i + 1][0] - screen[0][0];
    int by = screen[i + 1][1] - screen[0][1];
    int area2 = ax * by - ay * bx;
    if (area2 <= 0)
      continue;

    if (debug)
      draw_wireframe_tri_to_backbuffer(surface, screen[0], screen[i],
                                       screen[i + 1], r, g, b, 1);
    else
      draw_tri_to_backbuffer(surface, screen[0], screen[i], screen[i + 1],
                             normal_rgb[0], normal_rgb[1], normal_rgb[2], 0);
  }
}

void draw_tri3d_to_backbuffer_zbuffered(SDL_Surface *surface, uint32_t *zbuffer,
                                        camera c, vec3 v1, vec3 v2, vec3 v3,
                                        uint8_t r, uint8_t g, uint8_t b,
                                        vec3 pos, vec3 rot, vec3 pivot,
                                        int debug) {

  mat4 model, view, proj, mv, mvp;
  update_model_matrix(&model, pos, pivot, rot);
  update_view_matrix(&view, c);
  update_projection_matrix(&proj, c, surface->w, surface->h);
  mat4_mul(mv, view, model);
  mat4_mul(mvp, proj, mv);

  vec4 clip1, clip2, clip3;
  mat4_transform_clip(clip1, v1, mvp);
  mat4_transform_clip(clip2, v2, mvp);
  mat4_transform_clip(clip3, v3, mvp);

  vec3 normal;
  vec3 edge1, edge2;

  edge1[0] = v2[0] - v1[0];
  edge1[1] = v2[1] - v1[1];
  edge1[2] = v2[2] - v1[2];
  edge2[0] = v3[0] - v1[0];
  edge2[1] = v3[1] - v1[1];
  edge2[2] = v3[2] - v1[2];

  normal[0] = edge1[1] * edge2[2] - edge1[2] * edge2[1];
  normal[1] = edge1[2] * edge2[0] - edge1[0] * edge2[2];
  normal[2] = edge1[0] * edge2[1] - edge1[1] * edge2[0];

  float l = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] +
                  normal[2] * normal[2]);

  normal[0] /= l;
  normal[1] /= l;
  normal[2] /= l;

  mat4 model_inv;
  mat4_inverse(model_inv, model);
  mat4 normal_matrix;
  mat4_transpose(normal_matrix, model_inv);

  vec3 normal_world;
  mat4_vec3_mul(normal_world, normal_matrix, normal);

  if (clip1[3] <= 0 || clip2[3] <= 0 || clip3[3] <= 0)
    return;

  clip_vertex input_verts[6], output_verts[6];
  int count = 3;
  input_verts[0] =
      (clip_vertex){.p = {clip1[0], clip1[1], clip1[2]}, .w = clip1[3]};
  input_verts[1] =
      (clip_vertex){.p = {clip2[0], clip2[1], clip2[2]}, .w = clip2[3]};
  input_verts[2] =
      (clip_vertex){.p = {clip3[0], clip3[1], clip3[2]}, .w = clip3[3]};

  clip_vertex temp[6];
  int temp_count;

  clip_polygon_component(input_verts, count, output_verts, &temp_count, 0, 0);
  if (temp_count < 3)
    return;
  clip_polygon_component(output_verts, temp_count, temp, &count, 0, 1);
  if (count < 3)
    return;
  clip_polygon_component(temp, count, output_verts, &temp_count, 1, 0);
  if (temp_count < 3)
    return;
  clip_polygon_component(output_verts, temp_count, temp, &count, 1, 1);
  if (count < 3)
    return;
  clip_polygon_component(temp, count, output_verts, &temp_count, 2, 1);
  if (temp_count < 3)
    return;

  clip_vertex verts[6];
  count = temp_count;
  for (int i = 0; i < count; ++i)
    verts[i] = output_verts[i];

  float oow[6];
  float z_over_w[6];
  for (int i = 0; i < count; ++i) {
    float w = verts[i].w;
    if (w > 0.0001f) {
      oow[i] = 1.0f / w;
      z_over_w[i] = verts[i].p[2] * oow[i];
      verts[i].p[0] *= oow[i];
      verts[i].p[1] *= oow[i];
      verts[i].p[2] *= oow[i];
    } else {
      oow[i] = 0.0f;
      z_over_w[i] = 0.0f;
      verts[i].p[0] = verts[i].p[1] = verts[i].p[2] = 0.0f;
    }
  }

  vec2i screen[6];
  for (int i = 0; i < count; ++i) {
    float ndc_x = verts[i].p[0];
    float ndc_y = verts[i].p[1];

    float x = (ndc_x * 0.5f + 0.5f) * surface->w;
    float y = (1.0f - (ndc_y * 0.5f + 0.5f)) * surface->h;

    int ix = (int)x;
    int iy = (int)y;

    ix = ix < 0 ? 0 : (ix >= surface->w ? surface->w - 1 : ix);
    iy = iy < 0 ? 0 : (iy >= surface->h ? surface->h - 1 : iy);

    screen[i][0] = ix;
    screen[i][1] = iy;
  }

  vec3 normal_rgb;
  float r_f = (normal_world[0] + 1.0f) * 0.5f * 255.0f;
  float g_f = (normal_world[1] + 1.0f) * 0.5f * 255.0f;
  float b_f = (normal_world[2] + 1.0f) * 0.5f * 255.0f;

  normal_rgb[0] = r_f < 0.0f ? 0 : (r_f > 255.0f ? 255 : (uint8_t)r_f);
  normal_rgb[1] = g_f < 0.0f ? 0 : (g_f > 255.0f ? 255 : (uint8_t)g_f);
  normal_rgb[2] = b_f < 0.0f ? 0 : (b_f > 255.0f ? 255 : (uint8_t)b_f);

  for (int i = 1; i < count - 1; ++i) {
    int ax = screen[i][0] - screen[0][0];
    int ay = screen[i][1] - screen[0][1];
    int bx = screen[i + 1][0] - screen[0][0];
    int by = screen[i + 1][1] - screen[0][1];
    int area2 = ax * by - ay * bx;
    if (area2 <= 0)
      continue;

    if (debug)
      draw_wireframe_tri_to_backbuffer(surface, screen[0], screen[i],
                                       screen[i + 1], r, g, b, 1);
    else
      draw_tri_to_backbuffer_zbuffered(
          surface, zbuffer, screen[0], screen[i], screen[i + 1], normal_rgb[0],
          normal_rgb[1], normal_rgb[2], z_over_w[0], oow[0], z_over_w[i],
          oow[i], z_over_w[i + 1], oow[i + 1]);
  }
}
