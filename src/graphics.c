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

static inline void mat4_identity(mat4 m) {
  memset(m, 0, sizeof(mat4));
  m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

static inline void mat4_mul(mat4 out, const mat4 a, const mat4 b) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      out[i][j] = a[0][j] * b[i][0] + a[1][j] * b[i][1] + a[2][j] * b[i][2] +
                  a[3][j] * b[i][3];
    }
}

static inline void mat4_transform_clip(vec4 out, vec3 v, mat4 m) {
  float x = v[0], y = v[1], z = v[2];
  out[0] = x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0];
  out[1] = x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1];
  out[2] = x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2];
  out[3] = x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3];
}

static inline int clip_edge(clip_vertex *out1, clip_vertex *out2,
                            clip_vertex v1, clip_vertex v2) {
  float w1 = v1.w, w2 = v2.w;

  if (w1 > 0 && w2 > 0) {
    *out1 = v1;
    *out2 = v2;
    return 2;
  }
  if (w1 <= 0 && w2 <= 0)
    return 0;

  float t = w1 / (w1 - w2);
  if (t < 0.0f)
    t = 0.0f;
  if (t > 1.0f)
    t = 1.0f;

  clip_vertex clipped;
  clipped.p[0] = v1.p[0] + t * (v2.p[0] - v1.p[0]);
  clipped.p[1] = v1.p[1] + t * (v2.p[1] - v1.p[1]);
  clipped.p[2] = v1.p[2] + t * (v2.p[2] - v1.p[2]);
  clipped.w = v1.w + t * (v2.w - v1.w);

  if (clipped.w <= 0.0f)
    clipped.w = 0.0001f;

  if (w1 > 0) {
    *out1 = v1;
    *out2 = clipped;
  } else {
    *out1 = clipped;
    *out2 = v2;
  }
  return 2;
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

void update_model_matrix(mat4 *mat, vec3 pos, vec3 rot) {
  float cx = cosf(rot[0] * 3.14159265f / 180.0f);
  float sx = sinf(rot[0] * 3.14159265f / 180.0f);
  float cy = cosf(rot[1] * 3.14159265f / 180.0f);
  float sy = sinf(rot[1] * 3.14159265f / 180.0f);
  float cz = cosf(rot[2] * 3.14159265f / 180.0f);
  float sz = sinf(rot[2] * 3.14159265f / 180.0f);

  mat4_identity(*mat);
  (*mat)[0][0] = cy * cz + sy * sx * sz;
  (*mat)[0][1] = cx * sz;
  (*mat)[0][2] = -sy * cz + cy * sx * sz;
  (*mat)[1][0] = -cy * sz + sy * sx * cz;
  (*mat)[1][1] = cx * cz;
  (*mat)[1][2] = sy * sz + cy * sx * cz;
  (*mat)[2][0] = -sx * sy;
  (*mat)[2][1] = sx;
  (*mat)[2][2] = cx * cy;
  (*mat)[3][0] = pos[0];
  (*mat)[3][1] = pos[1];
  (*mat)[3][2] = pos[2];
  (*mat)[3][3] = 1.0f;
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
    float det =
        (v2[1] - v3[1]) * (v1[0] - v3[0]) + (v3[0] - v2[0]) * (v1[1] - v3[1]);
    if (det == 0)
      return;
    float inv = 1.0f / det;
    for (uint16_t y = sy; y <= ey; ++y)
      for (uint16_t x = sx; x <= ex; ++x) {
        float u =
            ((v2[1] - v3[1]) * (x - v3[0]) + (v3[0] - v2[0]) * (y - v3[1])) *
            inv;
        float v =
            ((v3[1] - v1[1]) * (x - v3[0]) + (v1[0] - v3[0]) * (y - v3[1])) *
            inv;
        float w = 1.0f - u - v;
        if (u >= 0 && v >= 0 && w >= 0)
          set_pixel(surface, x, y, (uint8_t)(u * 255), (uint8_t)(v * 255),
                    (uint8_t)(w * 255));
      }
    return;
  }

  bbox2i bb = calculate_bbox2i_from_tri(v1, v2, v3);
  uint16_t sx = max(0, bb.min[0]), ex = min(surface->w - 1, bb.max[0]);
  uint16_t sy = max(0, bb.min[1]), ey = min(surface->h - 1, bb.max[1]);
  float det =
      (v2[1] - v3[1]) * (v1[0] - v3[0]) + (v3[0] - v2[0]) * (v1[1] - v3[1]);
  if (det == 0)
    return;
  float inv = 1.0f / det;
  for (uint16_t y = sy; y <= ey; ++y)
    for (uint16_t x = sx; x <= ex; ++x) {
      float u =
          ((v2[1] - v3[1]) * (x - v3[0]) + (v3[0] - v2[0]) * (y - v3[1])) * inv;
      float v =
          ((v3[1] - v1[1]) * (x - v3[0]) + (v1[0] - v3[0]) * (y - v3[1])) * inv;
      float w = 1.0f - u - v;
      if (u >= 0 && v >= 0 && w >= 0)
        set_pixel(surface, x, y, r, g, b);
    }
}

void draw_tri3d_to_backbuffer(SDL_Surface *surface, camera c, vec3 v1, vec3 v2,
                              vec3 v3, uint8_t r, uint8_t g, uint8_t b,
                              vec3 pos, vec3 rot, int debug) {
  mat4 model, view, proj, mv, mvp;
  update_model_matrix(&model, pos, rot);
  update_view_matrix(&view, c);
  update_projection_matrix(&proj, c, surface->w, surface->h);
  mat4_mul(mv, view, model);
  mat4_mul(mvp, proj, mv);

  vec4 clip1, clip2, clip3;
  mat4_transform_clip(clip1, v1, mvp);
  mat4_transform_clip(clip2, v2, mvp);
  mat4_transform_clip(clip3, v3, mvp);

  if (clip1[3] <= 0 || clip2[3] <= 0 || clip3[3] <= 0)
    return;

  clip_vertex verts[6];
  int count = 3;
  verts[0] = (clip_vertex){.p = {clip1[0], clip1[1], clip1[2]}, .w = clip1[3]};
  verts[1] = (clip_vertex){.p = {clip2[0], clip2[1], clip2[2]}, .w = clip2[3]};
  verts[2] = (clip_vertex){.p = {clip3[0], clip3[1], clip3[2]}, .w = clip3[3]};

  clip_vertex input[6], output[6];
  int in_count = count;
  int out_count = 0;

  for (int i = 0; i < in_count; ++i)
    input[i] = verts[i];

  for (int i = 0; i < in_count; ++i) {
    clip_vertex a = input[i];
    clip_vertex b = input[(i + 1) % in_count];
    clip_vertex out1, out2;
    int n = clip_edge(&out1, &out2, a, b);

    if (n >= 1 && out_count < 5)
      output[out_count++] = out1;
    if (n == 2 && out_count < 5)
      output[out_count++] = out2;
    if (out_count >= 5)
      break;
  }

  count = out_count;
  for (int i = 0; i < count; ++i)
    verts[i] = output[i];

  if (count < 3)
    return;

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

    ndc_x = ndc_x < -1.0f ? -1.0f : (ndc_x > 1.0f ? 1.0f : ndc_x);
    ndc_y = ndc_y < -1.0f ? -1.0f : (ndc_y > 1.0f ? 1.0f : ndc_y);

    float x = (ndc_x * 0.5f + 0.5f) * surface->w;
    float y = (1.0f - (ndc_y * 0.5f + 0.5f)) * surface->h;

    int ix = (int)x;
    int iy = (int)y;

    ix = ix < 0 ? 0 : (ix >= surface->w ? surface->w - 1 : ix);
    iy = iy < 0 ? 0 : (iy >= surface->h ? surface->h - 1 : iy);

    screen[i][0] = ix;
    screen[i][1] = iy;
  }

  for (int i = 1; i < count - 1; ++i) {
    if (debug)
      draw_wireframe_tri_to_backbuffer(surface, screen[0], screen[i],
                                       screen[i + 1], r, g, b, 1);
    else
      draw_tri_to_backbuffer(surface, screen[0], screen[i], screen[i + 1], r, g,
                             b, 0);
  }
}
