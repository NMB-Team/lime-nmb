extern "C" {

	#define SPNG_STATIC
	#include <spng.h>
	#define PNG_SIG_SIZE 8

}

#include <graphics/format/PNG.h>
#include <graphics/ImageBuffer.h>
#include <system/System.h>
#include <utils/Bytes.h>
#include <utils/QuickVec.h>


namespace lime {


	bool PNG::Decode (Resource *resource, ImageBuffer *imageBuffer, bool decodeData) {

		spng_ctx *ctx = NULL;

		FILE_HANDLE* file = NULL;
		Bytes* data = NULL;
		int ret = 0;

		if (resource->path) {

			file = lime::fopen (resource->path, "rb");
			if (!file) return false;

			unsigned char png_sig[PNG_SIG_SIZE];
			int read = lime::fread (&png_sig, PNG_SIG_SIZE, 1, file);

			if (!read || png_sig[0] != 0x89 || png_sig[1] != 'P' || png_sig[2] != 'N' || png_sig[3] != 'G') {

				lime::fclose (file);
				return false;

			}

			#ifndef ANDROID
			lime::fseek (file, 0, SEEK_SET);
			#endif

		} else {

			const unsigned char *sig = resource->data->b;

			if (sig[0] != 0x89 || sig[1] != 'P' || sig[2] != 'N' || sig[3] != 'G') {

				return false;

			}

		}

		ctx = spng_ctx_new (0);

		if (!ctx) {

			if (file) lime::fclose (file);
			return false;

		}

		spng_set_crc_action (ctx, SPNG_CRC_USE, SPNG_CRC_USE);

		if (file) {

			if (file->isFile ()) {

				ret = spng_set_png_file (ctx, file->getFile ());

				if (ret) {

					spng_ctx_free (ctx);
					lime::fclose (file);
					return false;

				}

			} else {

				data = new Bytes ();
				data->ReadFile (resource->path);
				spng_set_png_buffer(ctx, data->b, data->length);

			}

		} else {

			spng_set_png_buffer(ctx, resource->data->b, resource->data->length);

		}

		struct spng_ihdr ihdr;
		ret = spng_get_ihdr (ctx, &ihdr);

		if (ret) {

			spng_ctx_free (ctx);

			if (file) lime::fclose (file);
			if (data) delete data;

			return false;

		}

		if (decodeData) {

			size_t out_size;

			ret = spng_decoded_image_size (ctx, SPNG_FMT_RGBA8, &out_size);

			if (ret) {

				spng_ctx_free (ctx);

				if (file) lime::fclose (file);
				if (data) delete data;

				return false;

			}

			imageBuffer->Resize (ihdr.width, ihdr.height, 32);

			ret = spng_decode_image (ctx, imageBuffer->data->buffer->b, out_size, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS);

			if (ret) {

				spng_ctx_free (ctx);

				if (file) lime::fclose (file);
				if (data) delete data;

				return false;

			}

		} else {

			imageBuffer->width = ihdr.width;
			imageBuffer->height = ihdr.height;

		}

		spng_ctx_free (ctx);

		if (file) lime::fclose (file);
		if (data) delete data;

		return true;

	}


	bool PNG::Encode (ImageBuffer *imageBuffer, Bytes* bytes) {

		spng_ctx *ctx = spng_ctx_new (SPNG_CTX_ENCODER);

		if (!ctx) {

			return false;

		}

		int ret = spng_set_option (ctx, SPNG_ENCODE_TO_BUFFER, 1);

		if (ret) {

			spng_ctx_free (ctx);
			return false;

		}

		struct spng_ihdr ihdr = {0};
		ihdr.width = imageBuffer->width;
		ihdr.height = imageBuffer->height;
		ihdr.bit_depth = 8;
		ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
		ret = spng_set_ihdr (ctx, &ihdr);

		if (ret) {

			spng_ctx_free (ctx);
			return false;

		}

		QuickVec<unsigned char> temp_buffer;

		temp_buffer.reserve (ihdr.width * ihdr.height * 4);

		for (int y = 0; y < ihdr.height; y++) {

			temp_buffer.append (imageBuffer->data->buffer->b + (imageBuffer->Stride() + y), ihdr.width * 4);

		}

		ret = spng_encode_image (ctx, &temp_buffer[0], ihdr.width * ihdr.height * 4, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);

		if (ret) {

			spng_ctx_free (ctx);
			return false;

		}

		size_t out_size;

		void *out_buffer = spng_get_png_buffer (ctx, &out_size, &ret);

		if (out_buffer == NULL) {

			spng_ctx_free (ctx);
			return false;

		}

		if (out_size > 0) {

			bytes->Resize (out_size);
			memcpy (bytes->b, out_buffer, out_size);

		}

		free (out_buffer);

		spng_ctx_free (ctx);

		return true;

	}

}