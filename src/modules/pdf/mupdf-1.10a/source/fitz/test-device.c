#include <mupdf/fitz.h>

typedef struct fz_test_device_s
{
	fz_device super;
	int *is_color;
	float threshold;
	int options;
	fz_device *passthrough;
	int resolved;
} fz_test_device;

static int
is_rgb_color(float threshold, float r, float g, float b)
{
	float rg_diff = fz_abs(r - g);
	float rb_diff = fz_abs(r - b);
	float gb_diff = fz_abs(g - b);
	return rg_diff > threshold || rb_diff > threshold || gb_diff > threshold;
}

static int
is_rgb_color_u8(int threshold_u8, int r, int g, int b)
{
	int rg_diff = fz_absi(r - g);
	int rb_diff = fz_absi(r - b);
	int gb_diff = fz_absi(g - b);
	return rg_diff > threshold_u8 || rb_diff > threshold_u8 || gb_diff > threshold_u8;
}

static void
fz_test_color(fz_context *ctx, fz_test_device *t, fz_colorspace *colorspace, const float *color)
{
	if (!*t->is_color && colorspace && colorspace != fz_device_gray(ctx))
	{
		if (colorspace == fz_device_rgb(ctx))
		{
			if (is_rgb_color(t->threshold, color[0], color[1], color[2]))
			{
				*t->is_color = 2;
				t->resolved = 1;
				if (t->passthrough == NULL)
					fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
			}
		}
		else
		{
			float rgb[3];
			fz_convert_color(ctx, fz_device_rgb(ctx), rgb, colorspace, color);
			if (is_rgb_color(t->threshold, rgb[0], rgb[1], rgb[2]))
			{
				*t->is_color = 2;
				t->resolved = 1;
				if (t->passthrough == NULL)
				{
					t->super.hints |= FZ_IGNORE_IMAGE;
					fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
				}
			}
		}
	}
}

static void
fz_test_fill_path(fz_context *ctx, fz_device *dev_, const fz_path *path, int even_odd, const fz_matrix *ctm,
	fz_colorspace *colorspace, const float *color, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	if (dev->resolved == 0)
	{
		if (alpha != 0.0f)
			fz_test_color(ctx, dev, colorspace, color);
	}
	if (dev->passthrough)
		fz_fill_path(ctx, dev->passthrough, path, even_odd, ctm, colorspace, color, alpha);
}

static void
fz_test_stroke_path(fz_context *ctx, fz_device *dev_, const fz_path *path, const fz_stroke_state *stroke,
	const fz_matrix *ctm, fz_colorspace *colorspace, const float *color, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	if (dev->resolved == 0)
	{
		if (alpha != 0.0f)
			fz_test_color(ctx, dev, colorspace, color);
	}
	if (dev->passthrough)
		fz_stroke_path(ctx, dev->passthrough, path, stroke, ctm, colorspace, color, alpha);
}

static void
fz_test_fill_text(fz_context *ctx, fz_device *dev_, const fz_text *text, const fz_matrix *ctm,
	fz_colorspace *colorspace, const float *color, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	if (dev->resolved == 0)
	{
		if (alpha != 0.0f)
			fz_test_color(ctx, dev, colorspace, color);
	}
	if (dev->passthrough)
		fz_fill_text(ctx, dev->passthrough, text, ctm, colorspace, color, alpha);
}

static void
fz_test_stroke_text(fz_context *ctx, fz_device *dev_, const fz_text *text, const fz_stroke_state *stroke,
	const fz_matrix *ctm, fz_colorspace *colorspace, const float *color, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	if (dev->resolved == 0)
	{
		if (alpha != 0.0f)
			fz_test_color(ctx, dev, colorspace, color);
	}
	if (dev->passthrough)
		fz_stroke_text(ctx, dev->passthrough, text, stroke, ctm, colorspace, color, alpha);
}

struct shadearg
{
	fz_test_device *dev;
	fz_shade *shade;
};

static void
prepare_vertex(fz_context *ctx, void *arg_, fz_vertex *v, const float *color)
{
	struct shadearg *arg = arg_;
	fz_test_device *dev = arg->dev;
	fz_shade *shade = arg->shade;
	if (!shade->use_function)
		fz_test_color(ctx, dev, shade->colorspace, color);
}

static void
fz_test_fill_shade(fz_context *ctx, fz_device *dev_, fz_shade *shade, const fz_matrix *ctm, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	if (dev->resolved == 0)
	{
		if ((dev->options & FZ_TEST_OPT_SHADINGS) == 0)
		{
			if (shade->colorspace != fz_device_gray(ctx))
			{
				/* Don't test every pixel. Upgrade us from "black and white" to "probably color" */
				if (*dev->is_color == 0)
					*dev->is_color = 1;
				dev->resolved = 1;
				if (dev->passthrough == NULL)
					fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
			}
		}
		else
		{
			if (shade->use_function)
			{
				int i;
				for (i = 0; i < 256; i++)
					fz_test_color(ctx, dev, shade->colorspace, shade->function[i]);
			}
			else
			{
				struct shadearg arg;
				arg.dev = dev;
				arg.shade = shade;
				fz_process_shade(ctx, shade, ctm, prepare_vertex, NULL, &arg);
			}
		}
	}
	if (dev->passthrough)
		fz_fill_shade(ctx, dev->passthrough, shade, ctm, alpha);
}

static void
fz_test_fill_image(fz_context *ctx, fz_device *dev_, fz_image *image, const fz_matrix *ctm, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	while (dev->resolved == 0) /* So we can break out */
	{
		fz_pixmap *pix;
		unsigned int count, i, k, h, sa, ss;
		unsigned char *s;
		fz_compressed_buffer *buffer;

		if (*dev->is_color || !image->colorspace || image->colorspace == fz_device_gray(ctx))
			break;

		if ((dev->options & FZ_TEST_OPT_IMAGES) == 0)
		{
			/* Don't test every pixel. Upgrade us from "black and white" to "probably color" */
			if (*dev->is_color == 0)
				*dev->is_color = 1;
			dev->resolved = 1;
			if (dev->passthrough == NULL)
				fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
			break;
		}

		buffer = fz_compressed_image_buffer(ctx, image);
		if (buffer && image->bpc == 8)
		{
			fz_stream *stream = fz_open_compressed_buffer(ctx, buffer);
			count = (unsigned int)image->w * (unsigned int)image->h;
			if (image->colorspace == fz_device_rgb(ctx))
			{
				int threshold_u8 = dev->threshold * 255;
				for (i = 0; i < count; i++)
				{
					int r = fz_read_byte(ctx, stream);
					int g = fz_read_byte(ctx, stream);
					int b = fz_read_byte(ctx, stream);
					if (is_rgb_color_u8(threshold_u8, r, g, b))
					{
						*dev->is_color = 1;
						dev->resolved = 1;
						fz_drop_stream(ctx, stream);
						if (dev->passthrough == NULL)
							fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
						break;
					}
				}
			}
			else
			{
				fz_color_converter cc;
				unsigned int n = (unsigned int)image->n;

				fz_init_cached_color_converter(ctx, &cc, fz_device_rgb(ctx), image->colorspace);
				for (i = 0; i < count; i++)
				{
					float cs[FZ_MAX_COLORS];
					float ds[FZ_MAX_COLORS];

					for (k = 0; k < n; k++)
						cs[k] = fz_read_byte(ctx, stream) / 255.0f;

					cc.convert(ctx, &cc, ds, cs);

					if (is_rgb_color(dev->threshold, ds[0], ds[1], ds[2]))
					{
						*dev->is_color = 1;
						dev->resolved = 1;
						if (dev->passthrough == NULL)
						{
							fz_drop_stream(ctx, stream);
							fz_fin_cached_color_converter(ctx, &cc);
							fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
						}
						break;
					}
				}
				fz_fin_cached_color_converter(ctx, &cc);
			}
			fz_drop_stream(ctx, stream);
			break;
		}

		pix = fz_get_pixmap_from_image(ctx, image, NULL, NULL, 0, 0);
		if (pix == NULL) /* Should never happen really, but... */
			break;

		count = pix->w;
		h = pix->h;
		s = pix->samples;
		sa = pix->alpha;
		ss = pix->stride - pix->w * pix->n;

		if (pix->colorspace == fz_device_rgb(ctx))
		{
			int threshold_u8 = dev->threshold * 255;
			while (h--)
			{
				for (i = 0; i < count; i++)
				{
					if ((!sa || s[3] != 0) && is_rgb_color_u8(threshold_u8, s[0], s[1], s[2]))
					{
						*dev->is_color = 1;
						dev->resolved = 1;
						if (dev->passthrough == NULL)
						{
							fz_drop_pixmap(ctx, pix);
							fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
						}
						break;
					}
					s += 3 + sa;
				}
				s += ss;
			}
		}
		else
		{
			fz_color_converter cc;
			unsigned int n = (unsigned int)pix->n-1;

			fz_init_cached_color_converter(ctx, &cc, fz_device_rgb(ctx), pix->colorspace);
			while (h--)
			{
				for (i = 0; i < count; i++)
				{
					float cs[FZ_MAX_COLORS];
					float ds[FZ_MAX_COLORS];

					for (k = 0; k < n; k++)
						cs[k] = (*s++) / 255.0f;
					if (sa && *s++ == 0)
						continue;

					cc.convert(ctx, &cc, ds, cs);

					if (is_rgb_color(dev->threshold, ds[0], ds[1], ds[2]))
					{
						*dev->is_color = 1;
						dev->resolved = 1;
						if (dev->passthrough == NULL)
						{
							fz_fin_cached_color_converter(ctx, &cc);
							fz_drop_pixmap(ctx, pix);
							fz_throw(ctx, FZ_ERROR_ABORT, "Page found as color; stopping interpretation");
						}
						break;
					}
				}
				s += ss;
			}
			fz_fin_cached_color_converter(ctx, &cc);
		}

		fz_drop_pixmap(ctx, pix);
		break;
	}
	if (dev->passthrough)
		fz_fill_image(ctx, dev->passthrough, image, ctm, alpha);
}

static void
fz_test_fill_image_mask(fz_context *ctx, fz_device *dev_, fz_image *image, const fz_matrix *ctm,
	fz_colorspace *colorspace, const float *color, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	if (dev->resolved == 0)
	{
		/* We assume that at least some of the image pixels are non-zero */
		fz_test_color(ctx, dev, colorspace, color);
	}
	if (dev->passthrough)
		fz_fill_image_mask(ctx, dev->passthrough, image, ctm, colorspace, color, alpha);
}

static void
fz_test_clip_path(fz_context *ctx, fz_device *dev_, const fz_path *path, int even_odd, const fz_matrix *ctm, const fz_rect *scissor)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_clip_path(ctx, dev->passthrough, path, even_odd, ctm, scissor);
}

static void
fz_test_clip_stroke_path(fz_context *ctx, fz_device *dev_, const fz_path *path, const fz_stroke_state *stroke, const fz_matrix *ctm, const fz_rect *scissor)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_clip_stroke_path(ctx, dev->passthrough, path, stroke, ctm, scissor);
}

static void
fz_test_clip_text(fz_context *ctx, fz_device *dev_, const fz_text *text, const fz_matrix *ctm, const fz_rect *scissor)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_clip_text(ctx, dev->passthrough, text, ctm, scissor);
}

static void
fz_test_clip_stroke_text(fz_context *ctx, fz_device *dev_, const fz_text *text, const fz_stroke_state *stroke, const fz_matrix *ctm, const fz_rect *scissor)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_clip_stroke_text(ctx, dev->passthrough, text, stroke, ctm, scissor);
}

static void
fz_test_ignore_text(fz_context *ctx, fz_device *dev_, const fz_text *text, const fz_matrix *ctm)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_ignore_text(ctx, dev->passthrough, text, ctm);
}

static void
fz_test_clip_image_mask(fz_context *ctx, fz_device *dev_, fz_image *img, const fz_matrix *ctm, const fz_rect *scissor)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_clip_image_mask(ctx, dev->passthrough, img, ctm, scissor);
}

static void
fz_test_pop_clip(fz_context *ctx, fz_device *dev_)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_pop_clip(ctx, dev->passthrough);
}

static void
fz_test_begin_mask(fz_context *ctx, fz_device *dev_, const fz_rect *rect, int luminosity, fz_colorspace *cs, const float *bc)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_begin_mask(ctx, dev->passthrough, rect, luminosity, cs, bc);
}

static void
fz_test_end_mask(fz_context *ctx, fz_device *dev_)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_end_mask(ctx, dev->passthrough);
}

static void
fz_test_begin_group(fz_context *ctx, fz_device *dev_, const fz_rect *rect, int isolated, int knockout, int blendmode, float alpha)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_begin_group(ctx, dev->passthrough, rect, isolated, knockout, blendmode, alpha);
}

static void
fz_test_end_group(fz_context *ctx, fz_device *dev_)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_end_group(ctx, dev->passthrough);
}

static int
fz_test_begin_tile(fz_context *ctx, fz_device *dev_, const fz_rect *area, const fz_rect *view, float xstep, float ystep, const fz_matrix *ctm, int id)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	return fz_begin_tile_id(ctx, dev->passthrough, area, view, xstep, ystep, ctm, id);
}

static void
fz_test_end_tile(fz_context *ctx, fz_device *dev_)
{
	fz_test_device *dev = (fz_test_device*)dev_;

	fz_end_tile(ctx, dev->passthrough);
}

fz_device *
fz_new_test_device(fz_context *ctx, int *is_color, float threshold, int options, fz_device *passthrough)
{
	fz_test_device *dev = fz_new_device(ctx, sizeof *dev);

	dev->super.fill_path = fz_test_fill_path;
	dev->super.stroke_path = fz_test_stroke_path;
	dev->super.fill_text = fz_test_fill_text;
	dev->super.stroke_text = fz_test_stroke_text;
	dev->super.fill_shade = fz_test_fill_shade;
	dev->super.fill_image = fz_test_fill_image;
	dev->super.fill_image_mask = fz_test_fill_image_mask;

	if (passthrough)
	{
		dev->super.clip_path = fz_test_clip_path;
		dev->super.clip_stroke_path = fz_test_clip_stroke_path;
		dev->super.clip_text = fz_test_clip_text;
		dev->super.clip_stroke_text = fz_test_clip_stroke_text;
		dev->super.ignore_text = fz_test_ignore_text;
		dev->super.clip_image_mask = fz_test_clip_image_mask;
		dev->super.pop_clip = fz_test_pop_clip;
		dev->super.begin_mask = fz_test_begin_mask;
		dev->super.end_mask = fz_test_end_mask;
		dev->super.begin_group = fz_test_begin_group;
		dev->super.end_group = fz_test_end_group;
		dev->super.begin_tile = fz_test_begin_tile;
		dev->super.end_tile = fz_test_end_tile;
	}

	dev->is_color = is_color;
	dev->options = options;
	dev->threshold = threshold;
	dev->passthrough = passthrough;
	dev->resolved = 0;

	*dev->is_color = 0;

	return (fz_device*)dev;
}
