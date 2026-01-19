#include <graphics/utils/ImageDataUtil.h>
#include <math/color/RGBA.h>
#include <utils/QuickVec.h>
#include <math.h>


namespace lime {


	unsigned char alphaTable[256];
	unsigned char redTable[256];
	unsigned char greenTable[256];
	unsigned char blueTable[256];


	void ImageDataUtil::ColorTransform (Image* image, Rectangle* rect, ColorMatrix* colorMatrix) {

		PixelFormat format = image->buffer->format;
		bool premultiplied = image->buffer->premultiplied;
		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;

		ImageDataView dataView = ImageDataView (image, rect);

		colorMatrix->GetAlphaTable (alphaTable);
		colorMatrix->GetRedTable (redTable);
		colorMatrix->GetGreenTable (greenTable);
		colorMatrix->GetBlueTable (blueTable);

		RGBA pixel;
		int width = dataView.width;

		for (int y = 0; y < dataView.height; y++) {

			uint8_t* rowPtr = data + dataView.Row (y);

			for (int x = 0; x < width; x++) {

				pixel.ReadUInt8 (rowPtr, 0, format, premultiplied, LIME_BIG_ENDIAN);
				pixel.Set (redTable[pixel.r], greenTable[pixel.g], blueTable[pixel.b], alphaTable[pixel.a]);
				pixel.WriteUInt8 (rowPtr, 0, format, premultiplied);
				rowPtr += 4;

			}

		}

	}


	void ImageDataUtil::CopyChannel (Image* image, Image* sourceImage, Rectangle* sourceRect, Vector2* destPoint, int srcChannel, int destChannel) {

		uint8_t* srcData = (uint8_t*)sourceImage->buffer->data->buffer->b;
		uint8_t* destData = (uint8_t*)image->buffer->data->buffer->b;

		ImageDataView srcView = ImageDataView (sourceImage, sourceRect);
		Rectangle destRect = Rectangle (destPoint->x, destPoint->y, srcView.width, srcView.height);
		ImageDataView destView = ImageDataView (image, &destRect);

		PixelFormat srcFormat = sourceImage->buffer->format;
		PixelFormat destFormat = image->buffer->format;
		bool srcPremultiplied = sourceImage->buffer->premultiplied;
		bool destPremultiplied = image->buffer->premultiplied;

		RGBA srcPixel, destPixel;
		unsigned char value = 0;
		int width = destView.width;

		for (int y = 0; y < destView.height; y++) {

			uint8_t* srcPtr = srcData + srcView.Row (y);
			uint8_t* destPtr = destData + destView.Row (y);

			for (int x = 0; x < width; x++) {

				srcPixel.ReadUInt8 (srcPtr, 0, srcFormat, srcPremultiplied, LIME_BIG_ENDIAN);
				destPixel.ReadUInt8 (destPtr, 0, destFormat, destPremultiplied, LIME_BIG_ENDIAN);

				switch (srcChannel) {
					case 0: value = srcPixel.r; break;
					case 1: value = srcPixel.g; break;
					case 2: value = srcPixel.b; break;
					case 3: value = srcPixel.a; break;
				}

				switch (destChannel) {
					case 0: destPixel.r = value; break;
					case 1: destPixel.g = value; break;
					case 2: destPixel.b = value; break;
					case 3: destPixel.a = value; break;
				}

				destPixel.WriteUInt8 (destPtr, 0, destFormat, destPremultiplied);

				srcPtr += 4;
				destPtr += 4;

			}

		}

	}


	void ImageDataUtil::CopyPixels (Image* image, Image* sourceImage, Rectangle* sourceRect, Vector2* destPoint, Image* alphaImage, Vector2* alphaPoint, bool mergeAlpha) {

		uint8_t* sourceData = (uint8_t*)sourceImage->buffer->data->buffer->b;
		uint8_t* destData = (uint8_t*)image->buffer->data->buffer->b;

		if (!sourceData || !destData) return;

		ImageDataView sourceView = ImageDataView (sourceImage, sourceRect);
		Rectangle destRect = Rectangle (destPoint->x, destPoint->y, sourceView.width, sourceView.height);
		ImageDataView destView = ImageDataView (image, &destRect);

		PixelFormat sourceFormat = sourceImage->buffer->format;
		PixelFormat destFormat = image->buffer->format;

		float sourceAlpha, destAlpha, oneMinusSourceAlpha, blendAlpha;
		RGBA sourcePixel, destPixel;

		bool sourcePremultiplied = sourceImage->buffer->premultiplied;
		bool destPremultiplied = image->buffer->premultiplied;
		int sourceBytesPerPixel = sourceImage->buffer->bitsPerPixel >> 3;
		int destBytesPerPixel = image->buffer->bitsPerPixel >> 3;

		bool useAlphaImage = (alphaImage && alphaImage->buffer->transparent);
		bool blend = (mergeAlpha || (useAlphaImage && !image->buffer->transparent) || (!mergeAlpha && !image->buffer->transparent && sourceImage->buffer->transparent));

		int width = destView.width;
		const float inv255 = 1.0f / 255.0f;

		if (!useAlphaImage) {

			if (blend) {

				for (int y = 0; y < destView.height; y++) {

					uint8_t* srcPtr = sourceData + sourceView.Row (y);
					uint8_t* destPtr = destData + destView.Row (y);

					for (int x = 0; x < width; x++) {

						sourcePixel.ReadUInt8 (srcPtr, 0, sourceFormat, sourcePremultiplied, LIME_BIG_ENDIAN);
						destPixel.ReadUInt8 (destPtr, 0, destFormat, destPremultiplied, LIME_BIG_ENDIAN);

						sourceAlpha = sourcePixel.a * inv255;
						destAlpha = destPixel.a * inv255;
						oneMinusSourceAlpha = 1.0f - sourceAlpha;
						blendAlpha = sourceAlpha + (destAlpha * oneMinusSourceAlpha);

						if (blendAlpha == 0) {

							destPixel.Set (0, 0, 0, 0);

						} else {

							float invBlendAlpha = 1.0f / blendAlpha;
							float destAlphaOMS = destAlpha * oneMinusSourceAlpha;
							destPixel.r = __clamp[(int)(0.5f + (sourcePixel.r * sourceAlpha + destPixel.r * destAlphaOMS) * invBlendAlpha)];
							destPixel.g = __clamp[(int)(0.5f + (sourcePixel.g * sourceAlpha + destPixel.g * destAlphaOMS) * invBlendAlpha)];
							destPixel.b = __clamp[(int)(0.5f + (sourcePixel.b * sourceAlpha + destPixel.b * destAlphaOMS) * invBlendAlpha)];
							destPixel.a = __clamp[(int)(0.5f + blendAlpha * 255.0f)];

						}

						destPixel.WriteUInt8 (destPtr, 0, destFormat, destPremultiplied);

						srcPtr += 4;
						destPtr += 4;

					}

				}

			} else if (sourceFormat == destFormat && sourcePremultiplied == destPremultiplied && sourceBytesPerPixel == destBytesPerPixel) {

				int rowBytes = width * destBytesPerPixel;
				for (int y = 0; y < destView.height; y++) {

					memcpy (destData + destView.Row (y), sourceData + sourceView.Row (y), rowBytes);

				}

			} else {

				for (int y = 0; y < destView.height; y++) {

					uint8_t* srcPtr = sourceData + sourceView.Row (y);
					uint8_t* destPtr = destData + destView.Row (y);

					for (int x = 0; x < width; x++) {

						sourcePixel.ReadUInt8 (srcPtr, 0, sourceFormat, sourcePremultiplied, LIME_BIG_ENDIAN);
						sourcePixel.WriteUInt8 (destPtr, 0, destFormat, destPremultiplied);

						srcPtr += 4;
						destPtr += 4;

					}

				}

			}

		} else {

			uint8_t* alphaData = (uint8_t*)alphaImage->buffer->data->buffer->b;
			PixelFormat alphaFormat = alphaImage->buffer->format;
			RGBA alphaPixel;

			Rectangle alphaRect = Rectangle (sourceView.x + alphaPoint->x, sourceView.y + alphaPoint->y, sourceView.width, sourceView.height);
			ImageDataView alphaView = ImageDataView (alphaImage, &alphaRect);

			destView.Clip (destPoint->x, destPoint->y, alphaView.width, alphaView.height);
			width = destView.width;

			if (blend) {

				for (int y = 0; y < destView.height; y++) {

					uint8_t* srcPtr = sourceData + sourceView.Row (y);
					uint8_t* destPtr = destData + destView.Row (y);
					uint8_t* alphaPtr = alphaData + alphaView.Row (y);

					for (int x = 0; x < width; x++) {

						sourcePixel.ReadUInt8 (srcPtr, 0, sourceFormat, sourcePremultiplied, LIME_BIG_ENDIAN);
						destPixel.ReadUInt8 (destPtr, 0, destFormat, destPremultiplied, LIME_BIG_ENDIAN);
						alphaPixel.ReadUInt8 (alphaPtr, 0, alphaFormat, false, LIME_BIG_ENDIAN);

						sourceAlpha = (alphaPixel.a * inv255) * (sourcePixel.a * inv255);

						if (sourceAlpha > 0) {

							destAlpha = destPixel.a * inv255;
							oneMinusSourceAlpha = 1.0f - sourceAlpha;
							blendAlpha = sourceAlpha + (destAlpha * oneMinusSourceAlpha);

							float invBlendAlpha = 1.0f / blendAlpha;
							float destAlphaOMS = destAlpha * oneMinusSourceAlpha;
							destPixel.r = __clamp[(int)(0.5f + (sourcePixel.r * sourceAlpha + destPixel.r * destAlphaOMS) * invBlendAlpha)];
							destPixel.g = __clamp[(int)(0.5f + (sourcePixel.g * sourceAlpha + destPixel.g * destAlphaOMS) * invBlendAlpha)];
							destPixel.b = __clamp[(int)(0.5f + (sourcePixel.b * sourceAlpha + destPixel.b * destAlphaOMS) * invBlendAlpha)];
							destPixel.a = __clamp[(int)(0.5f + blendAlpha * 255.0f)];

							destPixel.WriteUInt8 (destPtr, 0, destFormat, destPremultiplied);

						}

						srcPtr += 4;
						destPtr += 4;
						alphaPtr += 4;

					}

				}

			} else {

				for (int y = 0; y < destView.height; y++) {

					uint8_t* srcPtr = sourceData + sourceView.Row (y);
					uint8_t* destPtr = destData + destView.Row (y);
					uint8_t* alphaPtr = alphaData + alphaView.Row (y);

					for (int x = 0; x < width; x++) {

						sourcePixel.ReadUInt8 (srcPtr, 0, sourceFormat, sourcePremultiplied, LIME_BIG_ENDIAN);
						alphaPixel.ReadUInt8 (alphaPtr, 0, alphaFormat, false, LIME_BIG_ENDIAN);

						sourcePixel.a = (int)(0.5f + (sourcePixel.a * alphaPixel.a * inv255));
						sourcePixel.WriteUInt8 (destPtr, 0, destFormat, destPremultiplied);

						srcPtr += 4;
						destPtr += 4;
						alphaPtr += 4;

					}

				}

			}

		}

	}


	void ImageDataUtil::FillRect (Image* image, Rectangle* rect, int32_t color) {

		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;
		PixelFormat format = image->buffer->format;
		bool premultiplied = image->buffer->premultiplied;
		RGBA fillColor (color);

		if (rect->x == 0 && rect->y == 0 && rect->width == image->width && rect->height == image->height) {

			if (fillColor.a == fillColor.r && fillColor.r == fillColor.g && fillColor.g == fillColor.b) {

				memset (data, fillColor.a, image->buffer->data->byteLength);
				return;

			}

		}

		ImageDataView dataView = ImageDataView (image, rect);

		if (premultiplied) fillColor.MultiplyAlpha ();

		int width = dataView.width;
		int rowBytes = width * 4;

		if (dataView.height > 0) {
			uint8_t* firstRow = data + dataView.Row (0);
			for (int x = 0; x < width; x++) {
				fillColor.WriteUInt8 (firstRow, x * 4, format, false);
			}

			for (int y = 1; y < dataView.height; y++) {
				memcpy (data + dataView.Row (y), firstRow, rowBytes);
			}
		}

	}


	void ImageDataUtil::FloodFill (Image* image, int x, int y, int32_t color) {

		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;
		PixelFormat format = image->buffer->format;
		bool premultiplied = image->buffer->premultiplied;

		RGBA fillColor (color);

		if (premultiplied) fillColor.MultiplyAlpha ();

		RGBA hitColor;
		hitColor.ReadUInt8 (data, ((y + image->offsetY) * (image->buffer->width * 4)) + ((x + image->offsetX) * 4), format, premultiplied, LIME_BIG_ENDIAN);

		if (!image->buffer->transparent) {

			fillColor.a = 0xFF;
			hitColor.a = 0xFF;

		}

		if (fillColor == hitColor) return;

		int dx[4] = { 0, -1, 1, 0 };
		int dy[4] = { -1, 0, 0, 1 };

		int minX = -image->offsetX;
		int minY = -image->offsetY;
		int maxX = minX + image->width;
		int maxY = minY + image->height;

		QuickVec<int> queue = QuickVec<int> ();
		queue.push_back (x);
		queue.push_back (y);

		int curPointX, curPointY, i, nextPointX, nextPointY, nextPointOffset;
		RGBA readColor;

		while (queue.size () > 0) {

			curPointY = queue.qpop ();
			curPointX = queue.qpop ();

			for (i = 0; i < 4; i++) {

				nextPointX = curPointX + dx[i];
				nextPointY = curPointY + dy[i];

				if (nextPointX < minX || nextPointY < minY || nextPointX >= maxX || nextPointY >= maxY) {

					continue;

				}

				nextPointOffset = (nextPointY * image->width + nextPointX) * 4;
				readColor.ReadUInt8 (data, nextPointOffset, format, premultiplied, LIME_BIG_ENDIAN);

				if (readColor == hitColor) {

					fillColor.WriteUInt8 (data, nextPointOffset, format, false);

					queue.push_back (nextPointX);
					queue.push_back (nextPointY);

				}

			}

		}

	}


	void ImageDataUtil::GetPixels (Image* image, Rectangle* rect, PixelFormat format, Bytes* pixels) {

		int length = (int)(rect->width * rect->height);
		pixels->Resize (length * 4);

		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;
		uint8_t* destData = (uint8_t*)pixels->b;

		PixelFormat sourceFormat = image->buffer->format;
		bool premultiplied = image->buffer->premultiplied;

		ImageDataView dataView = ImageDataView (image, rect);
		RGBA pixel;
		uint8_t* destPtr = destData;
		int width = dataView.width;

		for (int y = 0; y < dataView.height; y++) {

			uint8_t* srcPtr = data + dataView.Row (y);

			for (int x = 0; x < width; x++) {

				pixel.ReadUInt8 (srcPtr, 0, sourceFormat, premultiplied, LIME_BIG_ENDIAN);
				pixel.WriteUInt8 (destPtr, 0, format, false);

				srcPtr += 4;
				destPtr += 4;

			}

		}

	}


	void ImageDataUtil::Merge (Image* image, Image* sourceImage, Rectangle* sourceRect, Vector2* destPoint, int redMultiplier, int greenMultiplier, int blueMultiplier, int alphaMultiplier) {

		ImageDataView sourceView = ImageDataView (sourceImage, sourceRect);
		Rectangle destRect = Rectangle (destPoint->x, destPoint->y, sourceView.width, sourceView.height);
		ImageDataView destView = ImageDataView (image, &destRect);

		uint8_t* sourceData = (uint8_t*)sourceImage->buffer->data->buffer->b;
		uint8_t* destData = (uint8_t*)image->buffer->data->buffer->b;
		PixelFormat sourceFormat = sourceImage->buffer->format;
		PixelFormat destFormat = image->buffer->format;
		bool sourcePremultiplied = sourceImage->buffer->premultiplied;
		bool destPremultiplied = image->buffer->premultiplied;

		RGBA sourcePixel, destPixel;

		int redInv = 256 - redMultiplier;
		int greenInv = 256 - greenMultiplier;
		int blueInv = 256 - blueMultiplier;
		int alphaInv = 256 - alphaMultiplier;
		int width = destView.width;

		for (int y = 0; y < destView.height; y++) {

			uint8_t* srcPtr = sourceData + sourceView.Row (y);
			uint8_t* destPtr = destData + destView.Row (y);

			for (int x = 0; x < width; x++) {

				sourcePixel.ReadUInt8 (srcPtr, 0, sourceFormat, sourcePremultiplied, LIME_BIG_ENDIAN);
				destPixel.ReadUInt8 (destPtr, 0, destFormat, destPremultiplied, LIME_BIG_ENDIAN);

				destPixel.r = ((sourcePixel.r * redMultiplier) + (destPixel.r * redInv)) >> 8;
				destPixel.g = ((sourcePixel.g * greenMultiplier) + (destPixel.g * greenInv)) >> 8;
				destPixel.b = ((sourcePixel.b * blueMultiplier) + (destPixel.b * blueInv)) >> 8;
				destPixel.a = ((sourcePixel.a * alphaMultiplier) + (destPixel.a * alphaInv)) >> 8;

				destPixel.WriteUInt8 (destPtr, 0, destFormat, destPremultiplied);

				srcPtr += 4;
				destPtr += 4;

			}

		}

	}


	void ImageDataUtil::MultiplyAlpha (Image* image) {

		PixelFormat format = image->buffer->format;
		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;
		int length = image->buffer->data->length >> 2;
		RGBA pixel;
		uint8_t* ptr = data;

		for (int i = 0; i < length; i++) {

			pixel.ReadUInt8 (ptr, 0, format, false, LIME_BIG_ENDIAN);
			pixel.WriteUInt8 (ptr, 0, format, true);
			ptr += 4;

		}

	}


	void ImageDataUtil::Resize (Image* image, ImageBuffer* buffer, int newWidth, int newHeight) {

		int imageWidth = image->width;
		int imageHeight = image->height;

		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;
		uint8_t* newData = (uint8_t*)buffer->data->buffer->b;

		int sourceIndex, sourceIndexX, sourceIndexY, sourceIndexXY;
		int sourceX, sourceY;
		float u, v, uRatio, vRatio, uOpposite, vOpposite;

		float scaleX = (float)imageWidth / newWidth;
		float scaleY = (float)imageHeight / newHeight;
		int srcStride = imageWidth * 4;
		int maxSrcX = imageWidth - 1;
		int maxSrcY = imageHeight - 1;

		uint8_t* destPtr = newData;

		for (int y = 0; y < newHeight; y++) {

			v = (y + 0.5f) * scaleY - 0.5f;
			sourceY = (int)v;
			vRatio = v - sourceY;
			vOpposite = 1.0f - vRatio;

			int rowOffset = sourceY * srcStride;
			bool canStepY = sourceY < maxSrcY;

			for (int x = 0; x < newWidth; x++) {

				u = (x + 0.5f) * scaleX - 0.5f;
				sourceX = (int)u;
				uRatio = u - sourceX;
				uOpposite = 1.0f - uRatio;

				sourceIndex = rowOffset + sourceX * 4;

				sourceIndexX = sourceIndex;
				if (sourceX < maxSrcX) sourceIndexX += 4;

				sourceIndexY = sourceIndex;
				if (canStepY) sourceIndexY += srcStride;

				sourceIndexXY = sourceIndexY;
				if (sourceIndexX != sourceIndex) sourceIndexXY += 4;

				float w00 = uOpposite * vOpposite;
				float w10 = uRatio * vOpposite;
				float w01 = uOpposite * vRatio;
				float w11 = uRatio * vRatio;

				destPtr[0] = (int)(data[sourceIndex] * w00 + data[sourceIndexX] * w10 + data[sourceIndexY] * w01 + data[sourceIndexXY] * w11);
				destPtr[1] = (int)(data[sourceIndex + 1] * w00 + data[sourceIndexX + 1] * w10 + data[sourceIndexY + 1] * w01 + data[sourceIndexXY + 1] * w11);
				destPtr[2] = (int)(data[sourceIndex + 2] * w00 + data[sourceIndexX + 2] * w10 + data[sourceIndexY + 2] * w01 + data[sourceIndexXY + 2] * w11);

				if (data[sourceIndexX + 3] == 0 || data[sourceIndexY + 3] == 0 || data[sourceIndexXY + 3] == 0) {
					destPtr[3] = 0;
				} else {
					destPtr[3] = data[sourceIndex + 3];
				}

				destPtr += 4;

			}

		}

	}


	void ImageDataUtil::SetFormat (Image* image, PixelFormat format) {

		if (image->buffer->format == format) return;

		int length = image->buffer->data->length >> 2;
		int r1, g1, b1, a1, r2, g2, b2, a2;

		switch (image->buffer->format) {

			case RGBA32:
				r1 = 0; g1 = 1; b1 = 2; a1 = 3;
				break;

			case ARGB32:
				r1 = 1; g1 = 2; b1 = 3; a1 = 0;
				break;

			case BGRA32:
				r1 = 2; g1 = 1; b1 = 0; a1 = 3;
				break;

		}

		switch (format) {

			case RGBA32:
				r2 = 0; g2 = 1; b2 = 2; a2 = 3;
				break;

			case ARGB32:
				r2 = 1; g2 = 2; b2 = 3; a2 = 0;
				break;

			case BGRA32:
				r2 = 2; g2 = 1; b2 = 0; a2 = 3;
				break;

		}

		unsigned char* data = image->buffer->data->buffer->b;
		unsigned char* ptr = data;
		unsigned char r, g, b, a;

		for (int i = 0; i < length; i++) {

			r = ptr[r1];
			g = ptr[g1];
			b = ptr[b1];
			a = ptr[a1];

			ptr[r2] = r;
			ptr[g2] = g;
			ptr[b2] = b;
			ptr[a2] = a;

			ptr += 4;

		}

	}


	void ImageDataUtil::SetPixels (Image* image, Rectangle* rect, Bytes* bytes, int offset, PixelFormat format, Endian endian) {

		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;
		PixelFormat destFormat = image->buffer->format;
		bool premultiplied = image->buffer->premultiplied;
		ImageDataView dataView = ImageDataView (image, rect);
		RGBA pixel;

		uint8_t* srcPtr = (uint8_t*)bytes->b + offset;
		bool transparent = image->buffer->transparent;
		int width = dataView.width;

		for (int y = 0; y < dataView.height; y++) {

			uint8_t* destPtr = data + dataView.Row (y);

			for (int x = 0; x < width; x++) {

				pixel.ReadUInt8 (srcPtr, 0, format, false, endian);
				if (!transparent) pixel.a = 0xFF;
				pixel.WriteUInt8 (destPtr, 0, destFormat, premultiplied);

				srcPtr += 4;
				destPtr += 4;

			}

		}

	}


	inline int __pixelCompare(int32_t n1, int32_t n2) {

		uint32_t u1 = static_cast<uint32_t>(n1);
		uint32_t u2 = static_cast<uint32_t>(n2);

		if (u1 > u2) return 1;
		if (u1 < u2) return -1;

		return 0;

	}


	int ImageDataUtil::Threshold(Image* image, Image* sourceImage, Rectangle* sourceRect, Vector2* destPoint, int operation, int32_t threshold, int32_t color, int32_t mask, bool copySource) {

		RGBA _color(color);
		int hits = 0;

		uint8_t* srcData = (uint8_t*)sourceImage->buffer->data->buffer->b;
		uint8_t* destData = (uint8_t*)image->buffer->data->buffer->b;

		ImageDataView srcView(sourceImage, sourceRect);
		Rectangle destRect(destPoint->x, destPoint->y, srcView.width, srcView.height);
		ImageDataView destView(image, &destRect);

		PixelFormat srcFormat = sourceImage->buffer->format;
		PixelFormat destFormat = image->buffer->format;
		bool srcPremultiplied = sourceImage->buffer->premultiplied;
		bool destPremultiplied = image->buffer->premultiplied;

		int srcPosition, destPosition;
		RGBA srcPixel;

		uint32_t uThreshold = static_cast<uint32_t>(threshold);

		#define THRESHOLD_LOOP(COND) \
			for (int y = 0; y < destView.height; y++) { \
				srcPosition = srcView.Row(y); \
				destPosition = destView.Row(y); \
				for (int x = 0; x < destView.width; x++) { \
					srcPixel.ReadUInt8(srcData, srcPosition, srcFormat, srcPremultiplied, LIME_BIG_ENDIAN); \
					uint32_t val = (uint32_t)(srcPixel.Get() & mask); \
					\
					if (COND) { \
						_color.WriteUInt8(destData, destPosition, destFormat, destPremultiplied); \
						hits++; \
					} else if (copySource) { \
						srcPixel.WriteUInt8(destData, destPosition, destFormat, destPremultiplied); \
					} \
					srcPosition += 4; \
					destPosition += 4; \
				} \
			}

		switch (operation) {
			case 0: THRESHOLD_LOOP(val != uThreshold); break;
			case 1: THRESHOLD_LOOP(val == uThreshold); break;
			case 2: THRESHOLD_LOOP(val < uThreshold);  break;
			case 3: THRESHOLD_LOOP(val <= uThreshold); break;
			case 4: THRESHOLD_LOOP(val > uThreshold);  break;
			case 5: THRESHOLD_LOOP(val >= uThreshold); break;
		}

		#undef THRESHOLD_LOOP

		return hits;

	}


	void ImageDataUtil::UnmultiplyAlpha (Image* image) {

		PixelFormat format = image->buffer->format;
		uint8_t* data = (uint8_t*)image->buffer->data->buffer->b;
		int length = image->buffer->data->length >> 2;
		RGBA pixel;
		uint8_t* ptr = data;

		for (int i = 0; i < length; i++) {

			pixel.ReadUInt8 (ptr, 0, format, true, LIME_BIG_ENDIAN);
			pixel.WriteUInt8 (ptr, 0, format, false);
			ptr += 4;

		}

	}


	ImageDataView::ImageDataView (Image* image, Rectangle* rect) {

		this->image = image;
		this->rect = Rectangle(rect->x, rect->y, rect->width, rect->height);

		if (this->rect.x < 0) this->rect.x = 0;
		if (this->rect.y < 0) this->rect.y = 0;
		if (this->rect.x + this->rect.width > image->width) this->rect.width = image->width - this->rect.x;
		if (this->rect.y + this->rect.height > image->height) this->rect.height = image->height - this->rect.y;
		if (this->rect.width < 0) this->rect.width = 0;
		if (this->rect.height < 0) this->rect.height = 0;

		stride = image->buffer->Stride ();

		__Update ();


	}


	void ImageDataView::Clip (int x, int y, int width, int height) {

		rect.Contract (x, y, width, height);
		__Update ();

	}


	inline bool ImageDataView::HasRow (int y) {

		return (y >= 0 && y < height);

	}


	void ImageDataView::Offset (int x, int y) {

		if (x < 0) {

			rect.x += x;
			if (rect.x < 0) rect.x = 0;

		} else {

			rect.x += x;
			rect.width -= x;

		}

		if (y < 0) {

			rect.y += y;
			if (rect.y < 0) rect.y = 0;

		} else {

			rect.y += y;
			rect.height -= y;

		}

		__Update ();

	}


	inline int ImageDataView::Row (int y) {

		return byteOffset + stride * y;

	}


	inline void ImageDataView::__Update () {

		this->x = (int) ceil (rect.x);
		this->y = (int) ceil (rect.y);
		this->width = (int) floor (rect.width);
		this->height = (int) floor (rect.height);
		byteOffset = (stride * (this->y + image->offsetY)) + ((this->x + image->offsetX) * 4);

	}


}
