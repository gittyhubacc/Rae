#include <lua.h>
#include <stdlib.h>
#include <string.h>
#include <lauxlib.h>


#define RES_W (12)
#define RES_H (12)

typedef char pixel_t;

#define WHITE ((pixel_t)0)
#define BLACK ((pixel_t)1)

pixel_t *primitive_new_image()
{
	pixel_t *buffer = malloc(RES_W * RES_H * sizeof(pixel_t));
	memset(buffer, 0, RES_W * RES_H * sizeof(pixel_t));
	return buffer;
}

pixel_t primitive_get_pixel(pixel_t *im, int x, int y)
{
	return im[y * RES_W + x];
}

pixel_t primitive_set_pixel(pixel_t *im, int x, int y, pixel_t p)
{
	im[y * RES_W + x] = p;
	return p;
}

pixel_t primitive_or_pixel(pixel_t left, pixel_t right)
{
	return left || right;
}

pixel_t primitive_and_pixel(pixel_t left, pixel_t right)
{
	return left && right;
}

pixel_t primitive_not_pixel(pixel_t p)
{
	return !p;
}

pixel_t *primitive_shift_up(pixel_t *im)
{
	pixel_t *res = primitive_new_image();
	for (int y = 0; y < RES_H; y++) {
		for (int x = 0; x < RES_W; x++) {
			pixel_t p = (y < RES_H - 1)
				? primitive_get_pixel(im, x, y + 1)
				: WHITE;
			primitive_set_pixel(res, x, y, p);
		}
	}
	return res;
}

pixel_t *primitive_shift_down(pixel_t *im)
{
	pixel_t *res = primitive_new_image();
	for (int y = 0; y < RES_H; y++) {
		for (int x = 0; x < RES_W; x++) {
			pixel_t p = (y > 0)
				? primitive_get_pixel(im, x, y - 1)
				: WHITE;
			primitive_set_pixel(res, x, y, p);
		}
	}
	return res;
}

pixel_t *primitive_shift_left(pixel_t *im)
{
	pixel_t *res = primitive_new_image();
	for (int y = 0; y < RES_H; y++) {
		for (int x = 0; x < RES_W; x++) {
			pixel_t p = (x < RES_W - 1)
				? primitive_get_pixel(im, x + 1, y)
				: WHITE;
			primitive_set_pixel(res, x, y, p);
		}
	}
	return res;
}

pixel_t *primitive_shift_right(pixel_t *im)
{
	pixel_t *res = primitive_new_image();
	for (int y = 0; y < RES_H; y++) {
		for (int x = 0; x < RES_W; x++) {
			pixel_t p = (x > 0)
				? primitive_get_pixel(im, x - 1, y)
				: WHITE;
			primitive_set_pixel(res, x, y, p);
		}
	}
	return res;
}

extern pixel_t *filter_main(pixel_t *im);

static int l_new_image(lua_State *L)
{
	lua_pushlightuserdata(L, primitive_new_image());
	return 1;
}

static int l_get_pixel(lua_State *L)
{
	pixel_t *buffer = lua_touserdata(L, 1);
	int x = lua_tointeger(L, 2);
	int y = lua_tointeger(L, 3);
	lua_pushinteger(L, buffer[y * RES_W + x]);
	return 1;
}

static int l_set_pixel(lua_State *L)
{
	pixel_t *buffer = lua_touserdata(L, 1);
	int x = lua_tointeger(L, 2);
	int y = lua_tointeger(L, 3);
	pixel_t p = lua_tointeger(L, 4);
	buffer[y * RES_W + x] = p;
	return 1;
}


static int l_main_filter(lua_State *L)
{
	pixel_t *buffer = lua_touserdata(L, 1);
	lua_pushlightuserdata(L, filter_main(buffer));
	return 1;
}

static const struct luaL_Reg code[] = {
	{"new_image", l_new_image },
	{"set_pixel", l_set_pixel }, // (img, x, y, color)
	{"get_pixel", l_get_pixel }, // (img, x, y)
	{"main_filter", l_main_filter}, // (img)
	{NULL, NULL}
};

int luaopen_env(lua_State *L)
{
	luaL_newlib(L, code);
	lua_pushstring(L, "white");
	lua_pushinteger(L, 0);
	lua_settable(L, -3);

	lua_pushstring(L, "black");
	lua_pushinteger(L, 1);
	lua_settable(L, -3);
	return 1;
}
