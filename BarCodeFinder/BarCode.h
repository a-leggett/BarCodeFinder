#pragma once
#include <immintrin.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

///<summary>Configuration that defines whether a pixel is considered 'yellow'.</summary>
typedef struct YellowConfig
{
	///<summary>The maximum separation value between the red and the green channels.</summary>
	uint8_t maxRedGreenSeparation;

	///<summary>The maximum separation value between the red and the blue channels.</summary>
	uint8_t minRedBlueSeparation;

	///<summary>The minimum value for the red channel.</summary>
	uint8_t minRed;
} YellowConfig;

///<summary>Configuration that defines whether a pixel is considered 'yellow' using AVX data types.</summary>
typedef struct YellowConfigAVX
{
	///<summary>The separation between the red and green channels must be less than this value, stored in all epi32 positions.</summary>
	__m256i redGreenSeparationLessThan;

	///<summary>The separation between the red and blue channels must be larger than this value, stored in all epi32 positions.</summary>
	__m256i redBlueSeparationGreaterThan;

	///<summary>The red channel must be larger than this value, stored in all epi32 positions.</summary>
	__m256i redGreaterThan;
} YellowConfigAVX;

///<summary>Checks whether a pixel is considered 'yellow' as defined by the <see cref="YellowConfig"/>.</summary>
///<param name="r">The value of the pixel's red channel.</param>
///<param name="g">The value of the pixel's green channel.</param>
///<param name="b">The value of the pixel's blue channel.</param>
///<param name="config">The <see cref="YellowConfig"/> that defines when a pixel is considered 'yellow'.</param>
///<returns>True if the pixel is considered yellow, otherwise false.</returns>
__forceinline bool is_yellow(uint8_t r, uint8_t g, uint8_t b, YellowConfig config)
{
	uint8_t redGreenSeparation = abs(r - g);
	int redBlueSeparation = (int)r - (int)b;
	return redGreenSeparation <= config.maxRedGreenSeparation && redBlueSeparation >= config.minRedBlueSeparation && r >= config.minRed;
}

///<summary>Converts a <see cref="YellowConfig"/> to a <see cref="YellowConfigAVX"/>.</summary>
///<param name="config">The source <see cref="YellowConfig"/>.</param>
///<returns>The equivalent <see cref="YellowConfigAVX"/>.</returns>
__forceinline YellowConfigAVX to_avx(YellowConfig config)
{
	YellowConfigAVX ret;

	ret.redGreenSeparationLessThan = _mm256_set1_epi32(config.maxRedGreenSeparation + 1/*+1 to go from '<=' to '<'*/);
	ret.redBlueSeparationGreaterThan = _mm256_set1_epi32(config.minRedBlueSeparation - 1/*-1 to go from '>=' to '>'*/);
	ret.redGreaterThan = _mm256_set1_epi32(config.minRed - 1/*-1 to go from '>=' to '>'*/);

	return ret;
}

///<summary>Checks whether pixels are yellow within a group of 8.</summary>
///<param name="rgba8"><see cref="__m256i"/> containing the 8 pixels, stored in RGBA 8-bit format.</param>
///<param name="config">The <see cref="YellowConfigAVX"/> that defines when a pixel is considered 'yellow.'</param>
///<returns>A <see cref="__m256i"/> where each byte is 0 (no bits set) or -1 (all bits set) depending on whether
///the pixel that contains that byte's channel is considered 'yellow.' So for a given pixel (4 channels: Red, Green
///Blue, and Alpha), if that pixel in the <paramref name="rgba8"/> was considered 'yellow,' then all of those four
///channels will be set to true (all bits set). If the pixel was not considered 'yellow,' then all bits in
///those four channels will be clear.</returns>
__forceinline __m256i _are_yellow(__m256i rgba8, YellowConfigAVX config)
{
	//Get the individual RGB channels
	__m256i reds = _mm256_and_si256(rgba8, _mm256_set_epi8(0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1));
	__m256i greens = _mm256_and_si256(rgba8, _mm256_set_epi8(0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0));
	__m256i blues = _mm256_and_si256(rgba8, _mm256_set_epi8(0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0));

	//Move the bytes for greens and blues to the correct position (so we can treat them as epi32) [reds are already in the correct position]
	greens = _mm256_shuffle_epi8(greens, _mm256_set_epi8(0, 0, 0, 29, 0, 0, 0, 25, 0, 0, 0, 21, 0, 0, 0, 17, 0, 0, 0, 13, 0, 0, 0, 9, 0, 0, 0, 5, 0, 0, 0, 1));//Byte value at '0' is '0' due to mask above
	blues = _mm256_shuffle_epi8(blues, _mm256_set_epi8(0, 0, 0, 30, 0, 0, 0, 26, 0, 0, 0, 22, 0, 0, 0, 18, 0, 0, 0, 14, 0, 0, 0, 10, 0, 0, 0, 6, 0, 0, 0, 2));//Byte value at '0' is '0' due to mask above

	//From here on, we can treat reds, greens, and blues as epi32 values

	__m256i redSubGreen = _mm256_sub_epi32(reds, greens);
	__m256i redSubBlue = _mm256_sub_epi32(reds, blues);

	__m256i redPassed = _mm256_cmpgt_epi32(reds, config.redGreaterThan);
	__m256i redSubGreenPassed = _mm256_cmpgt_epi32(config.redGreenSeparationLessThan, redSubGreen);
	__m256i redSubBluePassed = _mm256_cmpgt_epi32(redSubBlue, config.redBlueSeparationGreaterThan);

	//Since we compared using epi32, all channels for each pixel are set to true or false.
	return _mm256_and_si256(redPassed, _mm256_and_si256(redSubGreenPassed, redSubBluePassed));
}

///<summary>Copies an image, replacing 'yellow' pixels with a specific color.</summary>
///<param name="rgba8Source">The source image, in RGBA 8-bit format.</param>
///<param name="rgba8Dest">The destination image, in RGBA 8-bit format. May be equal to <paramref name="rgba8Source"/>.</param>
///<param name="width">The width, in pixels, of the image.</param>
///<param name="height">The height, in pixels, of the image.</param>
///<param name="config">The <see cref="YellowConfig"/> that defines when a pixel is considered 'yellow.'</param>
///<param name="r">The red component of the 'new' pixel (which will replace the 'yellow' pixels).</param>
///<param name="g">The green component of the 'new' pixel (which will replace the 'yellow' pixels).</param>
///<param name="b">The blue component of the 'new' pixel (which will replace the 'yellow' pixels).</param>
void show_yellow(const uint8_t * rgba8Source, uint8_t * rgba8Dest, int width, int height, YellowConfig config, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	YellowConfigAVX configAvx = to_avx(config);
	const __m256i* groupSource = (const __m256i*)rgba8Source;
	__m256i* groupDest = (__m256i*)rgba8Dest;
	int pixelCount = width * height * 4/*RGBA*/;
	assert(pixelCount % 32 == 0);
	int m256iCount = pixelCount / (32);

	__m256i color = _mm256_set_epi8(a, b, g, r, a, b, g, r, a, b, g, r, a, b, g, r, a, b, g, r, a, b, g, r, a, b, g, r, a, b, g, r);

	for (int i = 0; i < m256iCount; i++)
	{
		__m256i results = _are_yellow(groupSource[i], configAvx);

		__m256i src = _mm256_andnot_si256(results, groupSource[i]);
		__m256i draw = _mm256_and_si256(results, color);

		groupDest[i] = _mm256_or_si256(src, draw);
	}
}

///<summary>Stores information about a line of 'yellow' pixels.</summary>
typedef struct YellowScanLine
{
	///<summary>The x-position where the first yellow pixel of the line is located.</summary>
	int start;

	///<summary>The x-position where the last yellow pixel of the line is located.</summary>
	int end;

	///<summary>The y-position of the line.</summary>
	int y;

	///<summary>Flag that is internally used to determine whether to temporarily ignore this <see cref="YellowScanLine"/>.
	///The application should not read from or write to this field.</summary>
	bool _ignore;
} YellowScanLine;

///<summary>Finds all lines of consecutive 'yellow' pixels in an image.</summary>
///<param name="rgba8">The image's pixels, stored in RGBA 8-bit format.</param>
///<param name="width">The width of the image, measured in pixels.</param>
///<param name="height">The height of the image, measured in pixels.</param>
///<param name="cfg">The <see cref="YellowConfig"/> that defines when a pixel is considered 'yellow.'</param>
///<param name="dst">The destination <see cref="YellowScanLine"/> buffer.</param>
///<param name="maxCount">The maximum number of <see cref="YellowScanLine"/>s that can be stored in the <paramref name="dst"/> buffer.</param>
///<returns>The number of <see cref="YellowScanLine"/>s that have been found.</returns>
size_t find_yellow_lines(const uint8_t * rgba8, int width, int height, YellowConfig cfg, YellowScanLine * dst, size_t maxCount)
{
	assert(width % 8 == 0);//Width must be divisible by 8 so it can fit inside the __m256i values

	YellowConfigAVX configAvx = to_avx(cfg);

	const __m256i * blocks = (const __m256i*)rgba8;
	size_t found = 0;
	for (int y = 0; y < height; y++)
	{
		bool onLine = false;
		YellowScanLine currentLine;
		for (int x = 0; x < width; x += 8/*There are 8 pixels per __m256i*/)
		{
			__m256i currentBlock = blocks[0];
			blocks++;

			__m256i yellowMask = _are_yellow(currentBlock, configAvx);
			bool noneYellow = _mm256_testz_si256(yellowMask, yellowMask) != 0;

			if (noneYellow)
			{
				//Very common case: There are no yellow pixels here.
				if (onLine)
				{
					//The line has ended
					if (found < maxCount)
						dst[found++] = currentLine;
					else
						return found;

					onLine = false;
				}
				//Else we have nothing to do, just skip to the next block
			}
			else
			{
				//At least one pixel on the current block is yellow
				for (int i = 0; i < 8/*8 pixels per __m256i block*/; i++)
				{
					bool isYellow = yellowMask.m256i_i32[i] != 0;
					if (isYellow)
					{
						if (onLine)
						{
							//Just expand the existing line
							currentLine.end++;
						}
						else
						{
							//Starting a new line
							currentLine.start = x + i;
							currentLine.end = currentLine.start;
							currentLine.y = y;
							currentLine._ignore = false;
							onLine = true;
						}
					}
					else
					{
						if (onLine)
						{
							//The line has ended
							if (found < maxCount)
								dst[found++] = currentLine;
							else
								return found;

							onLine = false;
						}
						//Else nothing to do, just continue to the next pixel
					}
				}
			}
		}

		//Now we are done searching horizontally, but maybe a yellow line spanned all the way to the last 'x' pixel.
		if (onLine)
		{
			//End the line
			if (found < maxCount)
				dst[found++] = currentLine;
			else
				return found;

			onLine = false;
		}
	}

	return found;
}

///<summary>Draws <see cref="YellowScanLine"s/> onto an image.</summary>
///<param name="rgba8">The image's pixels, stored in RGBA 8-bit format.</param>
///<param name="width">The width of the image,  measured in pixels.</param>
///<param name="height">The height of the image, measured in pixels.</param>
///<param name="lines">The buffer of <see cref="YellowScanLine"/>s to be drawn.</param>
///<param name="lineCount">The number of lines to draw.</param>
///<param name="r">The red component of the color of the lines that will be drawn.</param>
///<param name="g">The green component of the color of the lines that will be drawn.</param>
///<param name="b">The blue component of the color of the lines that will be drawn.</param>
///<param name="a">The alpha component of the color of the lines that will be drawn.</param>
void show_yellow_lines(uint8_t * rgba8, int width, int height, const YellowScanLine * lines, size_t lineCount, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	for (size_t i = 0; i < lineCount; i++)
	{
		YellowScanLine line = lines[i];
		for (int x = line.start; x <= line.end; x++)
		{
			int y = line.y;
			rgba8[((x + (y * width)) * 4) + 0] = r;
			rgba8[((x + (y * width)) * 4) + 1] = g;
			rgba8[((x + (y * width)) * 4) + 2] = b;
			rgba8[((x + (y * width)) * 4) + 3] = a;
		}
	}
}

///<summary>Rectangular bounding box around a region of yellow pixels.</summary>
///<remarks>Note that a <see cref="YellowBoundingBox"/> does not have any margin surrounding the yellow region.
///This means that the 'left' region has at least one yellow pixel, so does the 'top,' 'right,' and 'bottom' region.
///So if there is a horizontal line of yellow pixels with one pixel height, then a <see cref="YellowBoundingBox"/>
///around that line will have a height of zero (because <see cref="YellowBoundingBox.top"/> and <see cref="YellowBoundingBox.bottom"/>
///would be equal).</remarks>
typedef struct YellowBoundingBox
{
	///<summary>The left position of the bounding box.</summary>
	int left;

	///<summary>The top position of the bounding box.</summary>
	int top;

	///<summary>The right position of the bounding box.</summary>
	int right;

	///<summary>The bottom position of the bounding box.</summary>
	int bottom;

	///<summary>Has the entire bounding box been found completely? False indicates that it may be corrupt, and thus the
	///size of this <see cref="YellowBoundingBox"/> may be less than the actual size of the grouped <see cref="YellowScanLine"/>s.</summary>
	bool isComplete;
} YellowBoundingBox;

///<summary>Checks whether two <see cref="YellowScanLine"/>s are adjacent within a specified maximum spacing.</summary>
///<param name="a">The first <see cref="YellowScanLine"/>.</param>
///<param name="b">The second <see cref="YellowScanLine"/>.</param>
///<param name="maxSpacing">The maximum spacing between adjacent lines.</param>
///<returns>True if the two lines are adjacent within the specified <paramref name="maxSpacing"/>.</returns>
__forceinline bool _are_lines_adjacent(YellowScanLine a, YellowScanLine b, int maxSpacing)
{
	int verticalSpacing = abs(a.y - b.y);
	if (verticalSpacing > (maxSpacing + 1 /*Because we consider 'vertically touching' lines to have spacing of zero. Consider what happens if we don't do +1 while 'maxSpacing' is zero.*/))
		return false;

	if (a.start + maxSpacing >= b.start && a.start - maxSpacing <= b.end)
		return true;//a.start is completely enclosed by b

	if (b.start + maxSpacing >= a.start && b.start - maxSpacing <= a.end)
		return true;//b.start is completely enclosed by a

	if (a.end + maxSpacing >= b.start && a.end - maxSpacing <= b.end)
		return true;//a.end is completely enclosed by b

	if (b.end + maxSpacing >= a.start && b.end - maxSpacing <= a.end)
		return true;//b.end is completely enclosed by a

	return false;
}

///<summary>Finds <see cref="YellowBoundingBox"/>es for all grouped <see cref="YellowScanLine"/>s.</summary>
///<param name="lines">The <see cref="YellowScanLine"/>s.</param>
///<param name="lineCount">The number of <see cref="YellowScanLine"/>s in <paramref name="lines"/>.</param>
///<param name="maxSpacing">The maximum spacing between adjacent pixels.</param>
///<param name="tmpIndexBuffer">Buffer into which temporary indices will be stored. The caller does not need to initialize any value to this buffer,
///and it should expect garbage when this function ends.</param>
///<param name="maxTmpIndexCount">The maximum number of indices (<see cref="size_t"/> values) that can be stored in <paramref name="tmpIndexBuffer"/>.
///Note that this size directly affects the maximum number of <see cref="YellowScanLine"/>s that can make up a <see cref="YellowBoundingBox"/>, so it should
///be sufficiently large. If there are any <see cref="YellowBoundingBox"/>es with more <see cref="YellowScanLine"/>s than can be indexed by the
///<paramref name="tmpIndexBuffer"/>, then those <see cref="YellowBoundingBox"/>es may be corrupt (Their <see cref="YellowBoundingBox.isComplete"/>
///field may be false and they may be smaller than the actual group of <see cref="YellowScanLine"/>s).</param>
///<param name="dst">The destination <see cref="YellowBoundingBox"/> buffer.</param>
///<param name="maxCount">The maximum number of <see cref="YellowScanLine"/>s to store in <paramref name="dst"/>.</param>
size_t find_yellow_rectangles(YellowScanLine * lines, size_t lineCount, int maxSpacing, size_t * tmpIndexBuffer, size_t maxTmpIndexCount, YellowBoundingBox * dst, size_t maxCount)
{
	//Reset the '_ignore' field of all lines
	for (size_t i = 0; i < lineCount; i++)
		lines[i]._ignore = false;

	size_t boxCount = 0;
	for (size_t i = 0; i < lineCount; i++)
	{
		if (lines[i]._ignore)
			continue;//We already scanned this line as a member of a previously-scanned YellowBoundingBox, so don't read it again, we would just get the same YellowBoundingBox.

		YellowBoundingBox currentBox;
		currentBox.left = lines[i].start;
		currentBox.top = lines[i].y;
		currentBox.right = lines[i].end;
		currentBox.bottom = lines[i].y;

		//Find all YellowScanLines that fit in 'currentBox' (starting from lines[i])
		size_t containedLineCount = 0;
		bool ranOutOfTempSpace = false;
		tmpIndexBuffer[containedLineCount++] = i;
		for (int j = 0; j < containedLineCount; j++)//Search through all 'contained' lines (this grows as we iterate, until bounding box is complete)
		{
			size_t currentIndex = tmpIndexBuffer[j];
			YellowScanLine currentLine = lines[currentIndex];

			lines[currentIndex]._ignore = true;//Make sure we don't duplicate this line in the future

			//Find all lines that are adjacent to 'currentLine'
			for (size_t k = i + 1; k < lineCount; k++)
			{
				if (lines[k]._ignore)
					continue;//Don't re-read this line
				if (k == currentIndex)
					continue;//Don't check whether a line is adjacent to itself

				if (_are_lines_adjacent(currentLine, lines[k], maxSpacing))
				{
					if (containedLineCount < maxTmpIndexCount)
					{
						tmpIndexBuffer[containedLineCount++] = k;
						lines[k]._ignore = true;
					}
					else
						ranOutOfTempSpace = true;//The 'tmpIndexBuffer' was too small!
				}
			}
		}

		//If we ran out of space in 'tmpIndexBuffer,' then the current bounding box is incomplete.
		currentBox.isComplete = !ranOutOfTempSpace;

		//Now we have the indices of all contained lines for this bounding box stored in 'tmpIndexBuffer,' so simply fit the bounding box
		//around all those lines
		for (int j = 0; j < containedLineCount; j++)
		{
			YellowScanLine currentLine = lines[tmpIndexBuffer[j]];

			if (currentLine.start < currentBox.left)
				currentBox.left = currentLine.start;
			if (currentLine.end > currentBox.right)
				currentBox.right = currentLine.end;
			if (currentLine.y < currentBox.top)
				currentBox.top = currentLine.y;
			if (currentLine.y > currentBox.bottom)
				currentBox.bottom = currentLine.y;

			//Make sure we don't read this line later (doing so would cause nested rectangles)
			lines[tmpIndexBuffer[j]]._ignore = true;
		}

		if (boxCount < maxCount)
			dst[boxCount++] = currentBox;
		else
			return boxCount;
	}

	return boxCount;
}

///<summary>Draws <see cref="YellowBoundingBox"/>es to an image.</summary>
///<param name="rgba8">The pixels of the image, stored in RGBA 8-bit format.</param>
///<param name="width">The width of the image, measured in pixels.</param>
///<param name="height">The height of the image, measured in pixels.</param>
///<param name="boxes">The <see cref="YellowBoundingBox"/>es to draw.</param>
///<param name="boxCount">The number of <see cref="YellowBoundingBox"/>es in <paramref name="boxes"/> to draw.</param>
///<param name="r">The red component of the color of the boxes to be drawn.</param>
///<param name="g">The green component of the color of the boxes to be drawn.</param>
///<param name="b">The blue component of the color of the boxes to be drawn.</param>
///<param name="a">The alpha component of the color of the boxes to be drawn.</param>
void show_yellow_rectangles(uint8_t * rgba8, int width, int height, const YellowBoundingBox * boxes, size_t boxCount, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	for (size_t i = 0; i < boxCount; i++)
	{
		//Draw the top and bottom lines
		for (int x = boxes[i].left; x <= boxes[i].right; x++)
		{
			//Top line
			rgba8[((x + (boxes[i].top * width)) * 4) + 0] = r;
			rgba8[((x + (boxes[i].top * width)) * 4) + 1] = g;
			rgba8[((x + (boxes[i].top * width)) * 4) + 2] = b;
			rgba8[((x + (boxes[i].top * width)) * 4) + 3] = a;

			//Bottom line
			rgba8[((x + (boxes[i].bottom * width)) * 4) + 0] = r;
			rgba8[((x + (boxes[i].bottom * width)) * 4) + 1] = g;
			rgba8[((x + (boxes[i].bottom * width)) * 4) + 2] = b;
			rgba8[((x + (boxes[i].bottom * width)) * 4) + 3] = a;
		}

		//Draw the left and right lines
		for (int y = boxes[i].top; y <= boxes[i].bottom; y++)
		{
			//Left line
			rgba8[((boxes[i].left + (y * width)) * 4) + 0] = r;
			rgba8[((boxes[i].left + (y * width)) * 4) + 1] = g;
			rgba8[((boxes[i].left + (y * width)) * 4) + 2] = b;
			rgba8[((boxes[i].left + (y * width)) * 4) + 3] = a;

			//Right line
			rgba8[((boxes[i].right + (y * width)) * 4) + 0] = r;
			rgba8[((boxes[i].right + (y * width)) * 4) + 1] = g;
			rgba8[((boxes[i].right + (y * width)) * 4) + 2] = b;
			rgba8[((boxes[i].right + (y * width)) * 4) + 3] = a;
		}
	}
}

///<summary>The maximum number of <see cref="BarCodeColor"/>s that can fit in a <see cref="BarCode"/>.</summary>
#define BAR_CODE_MAX_COLOR_COUNT (25)

///<summary>Defines a color within a bar code.</summary>
typedef enum BarCodeColor
{
	BAR_CODE_RED = 'R',
	BAR_CODE_GREEN = 'G',
	BAR_CODE_BLUE = 'B'
} BarCodeColor;

///<summary>Defines a sequence of <see cref="BarCodeColor"/>s.</summary>
typedef struct BarCode
{
	///<summary>The buffer of <see cref="BarCodeColor"/>s. Since there is no 'polarity' on an observed bar code, 
	///palindromes are considered equivalent. For example: {BAR_CODE_RED, BAR_CODE_GREEN, BAR_CODE_GREEN, BAR_CODE_BLUE} and 
	///{BAR_CODE_BLUE, BAR_CODE_GREEN, BAR_CODE_GREEN, BAR_CODE_RED} are equal.</summary>
	BarCodeColor colors[BAR_CODE_MAX_COLOR_COUNT];

	///<summary>The number of <see cref="BarCodeColor"/>s stored in <see cref="BarCode.colors"/>.
	///This value cannot be larger than <see cref="BAR_CODE_MAX_COLOR_COUNT"/>.</summary>
	size_t colorCount;
} BarCode;

///<summary>Defines the appearance information about a bar code.</summary>
typedef struct BarCodeAppearance
{
	///<summary>The <see cref="YellowBoundingBox"/> that contained the first 'yellow bar.'</summary>
	YellowBoundingBox _firstBox;

	///<summary>The <see cref="YellowBoundingBox"/> that contained the second 'yellow bar.'</summary>
	YellowBoundingBox _secondBox;

	///<summary>The x-coordinate where the 'color line' begins.</summary>
	int colorStartX;

	///<summary>The y-coordinate where the 'color line' begins.</summary>
	int colorStartY;

	///<summary>The x-coordinate where the 'color line' ends.</summary>
	int colorEndX;

	///<summary>The y-coordinate where the 'color line' ends.</summary>
	int colorEndY;

	///<summary>The number of sections in the bar code.</summary>
	int sectionCount;

	///<summary>The average rank for how 'red' a particular section of the bar code was, ranging from 0.0f to 1.0f.</summary>
	///<remarks>The number of values is determined by <see cref="sectionCount"/>.</remarks>
	float redAverage[BAR_CODE_MAX_COLOR_COUNT];

	///<summary>The average rank for how 'green' a particular section of the bar code was, ranging from 0.0f to 1.0f.</summary>
	///<remarks>The number of values is determined by <see cref="sectionCount"/>.</remarks>
	float greenAverage[BAR_CODE_MAX_COLOR_COUNT];

	///<summary>The average rank for how 'blue' a particular section of the bar code was, ranging from 0.0f to 1.0f.</summary>
	///<remarks>The number of values is determined by <see cref="sectionCount"/>.</remarks>
	float blueAverage[BAR_CODE_MAX_COLOR_COUNT];

	///<summary>The total number of pixels that were observed per section of the bar code.</summary>
	///<remarks>The number of values is determined by <see cref="sectionCount"/>.</remarks>
	int pixelCount[BAR_CODE_MAX_COLOR_COUNT];
} BarCodeAppearance;

__forceinline float _quantify_bar_code_appearance_match(const BarCode* code, const float* rAvg, const float* gAvg, const float* bAvg, bool reverse)
{
	size_t sectionCount = code->colorCount;
	size_t start = reverse ? sectionCount - 1 : 0;
	size_t increment = reverse ? -1 : 1;
	float sum = 0.0f;
	for (size_t scanI = start, codeI = 0; scanI >= 0 && scanI < sectionCount; scanI += increment, codeI++)
	{
		//scanI is to iterate the observed color values. If 'reverse' is true, we will iterate from end to start.
		//codeI is to iterate the expected color values. This value always iterates from 0 to 'sectionCount'.
		BarCodeColor expectedColor = code->colors[codeI];
		float r = rAvg[scanI];
		float g = gAvg[scanI];
		float b = bAvg[scanI];

		switch (expectedColor)
		{
		case BAR_CODE_RED:
			sum += r;
			sum -= g;
			sum -= b;
			break;

		case BAR_CODE_GREEN:
			sum += g;
			sum -= r;
			sum -= b;
			break;

		case BAR_CODE_BLUE:
			sum += b;
			sum -= r;
			sum -= g;
			break;

		default:
			assert(0 && "Unrecognized BarCodeColor value");
			break;
		}
	}

	float score = sum / sectionCount;
	if (score < 0.0f)
		score = 0.0f;
	if (score > 1.0f)
		score = 1.0f;
	return score;
}

///<summary>Quantifies how well a <see cref="BarCodeAppearance"/> matches a specific <see cref="BarCode"/>, ranging from
///0.0 (no match) to 1.0 (full match).</summary>
///<param name="code">The <see cref="BarCode"/>.</param>
///<param name="appearance">The <see cref="BarCodeAppearance"/>.</param>
///<returns>A value ranging from 0.0f to 1.0f, where 0.0f is no match and 1.0f is full match.</returns>
__forceinline float quantify_bar_code_appearance_match(const BarCode* code, const BarCodeAppearance* appearance)
{
	assert(code->colorCount == appearance->sectionCount);

	//Since bar codes do not have polarity, we have to 'scan' the line in two directions.
	float forward = _quantify_bar_code_appearance_match(code, appearance->redAverage, appearance->greenAverage, appearance->blueAverage, false);
	float reverse = _quantify_bar_code_appearance_match(code, appearance->redAverage, appearance->greenAverage, appearance->blueAverage, true);

	//We will take the best match of 'forward' or 'reverse.'
	if (forward > reverse)
		return forward;
	else
		return reverse;
}

///<summary>Quantifies how distinctly red a particular color is.</summary>
///<param name="r">The red component of the color.</param>
///<param name="g">The green component of the color.</param>
///<param name="b">The blue component of the color.</param>
///<returns>A value ranging from 0.0f to 1.0f, where 0.0f is 'not red' and 1.0f is 'full red.'</returns>
__forceinline float quantify_red(uint8_t r, uint8_t g, uint8_t b)
{
	return (r > g&& r > b)?1.0f:0.0f;//TODO: Tune this according to your specific needs (depending on camera, lighting, etc)
}

///<summary>Quantifies how distinctly green a particular color is.</summary>
///<param name="r">The red component of the color.</param>
///<param name="g">The green component of the color.</param>
///<param name="b">The blue component of the color.</param>
///<returns>A value ranging from 0.0f to 1.0f, where 0.0f is 'not green' and 1.0f is 'full green.'</returns>
__forceinline float quantify_green(uint8_t r, uint8_t g, uint8_t b)
{
	return (g > r&& g > b)?1.0f:0.0f;//TODO: Tune this according to your specific needs (depending on camera, lighting, etc)
}

///<summary>Quantifies how distinctly blue a particular color is.</summary>
///<param name="r">The red component of the color.</param>
///<param name="g">The green component of the color.</param>
///<param name="b">The blue component of the color.</param>
///<returns>A value ranging from 0.0f to 1.0f, where 0.0f is 'not blue' and 1.0f is 'full blue.'</returns>
__forceinline float quantify_blue(uint8_t r, uint8_t g, uint8_t b)
{
	return (b > r&& b > g)?1.0f:0.0f;//TODO: Tune this according to your specific needs (depending on camera, lighting, etc)
}

void _draw_line(uint8_t * rgba8, int width, int height, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = x1 < x2 ? 1 : -1;
	int sy = y1 < y2 ? 1 : -1;
	int error = (dx > dy ? dx : -dy) / 2;
	while (true) {

		rgba8[((x1 + (y1 * width)) * 4) + 0] = r;
		rgba8[((x1 + (y1 * width)) * 4) + 1] = g;
		rgba8[((x1 + (y1 * width)) * 4) + 2] = b;
		rgba8[((x1 + (y1 * width)) * 4) + 3] = a;

		if (x1 == x2 && y1 == y2)
			break;

		int errorCopy = error;
		if (errorCopy > -dx)
		{
			error -= dy;
			x1 += sx;
		}

		if (errorCopy < dy)
		{
			error += dx;
			y1 += sy;
		}
	}
}

///<summary>Given a line that starts and ends in 'yellow bars,' finds the endpoints of the 'colorful' line between the 'yellow bars.'</summary>
///<param name="rgba8">The image's pixels, stored in RGBA 8-bit format.</param>
///<param name="width">The width of the image, measured in pixels.</param>
///<param name="height">The height of the image, measured in pixels.</param>
///<param name="yellowCfg">The <see cref="YellowConfig"/> that identifies the 'yellow bars.'</param>
///<param name="firstX">As input, identifies the X position of the midpoint of the first 'yellow bar.' As output, identifies the X
///position where the 'colorful' line begins.</param>
///<param name="firstY">As input, identifies the Y position of the midpoint of the first 'yellow bar.' As output, identifies the Y
///position where the 'colorful' line begins.</param>
///<param name="secondX">As input, identifies the X position of the midpoint of the last 'yellow bar.' As output, identifies the X
///position where the 'colorful' line ends.</param>
///<param name="secondY">As input, identifies the Y position of the midpoint of the last 'yellow bar.' As output, identifies the Y
///position where the 'colorful' line ends.</param>
void _find_colorful_line_endpoints(const uint8_t * rgba8, int width, int height, YellowConfig yellowCfg, int* firstX, int* firstY, int* secondX, int* secondY)
{
	int x1 = firstX[0];
	int y1 = firstY[0];
	int x2 = secondX[0];
	int y2 = secondY[0];
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);
	int sx = x1 < x2 ? 1 : -1;
	int sy = y1 < y2 ? 1 : -1;
	int error = (dx > dy ? dx : -dy) / 2;
	bool foundColorStart = false;
	bool inColor = false;
	int lastX = x2, lastY = y2;
	while (true) {

		uint8_t r = rgba8[((x1 + (y1 * width)) * 4) + 0];
		uint8_t g = rgba8[((x1 + (y1 * width)) * 4) + 1];
		uint8_t b = rgba8[((x1 + (y1 * width)) * 4) + 2];

		if (is_yellow(r, g, b, yellowCfg))
		{
			if (inColor)
			{
				//We found the start of the 'end yellow bar', so stop just before here.
				//NOTE: We may be wrong! This may be an anomalous pixel on the line which happens to trigger the 'is yellow pixel' filter.
				//      This function will always take the LAST position just before a 'yellow bar' region. Usually there is only one (hopefully always!)
				secondX[0] = lastX;
				secondY[0] = lastY;
				inColor = false;
			}
			//else we are still in the 'start yellow bar', so do nothing
		}
		else
		{
			if (inColor)
			{
				//We are still in the 'colorful' part of the line, so keep searching
			}
			else
			{
				if (!foundColorStart)
				{
					//We found the start of the 'colorful' part of the line
					firstX[0] = x1;
					firstY[0] = y1;
					foundColorStart = true;
				}
				else
				{
					//This indicates an anomaly. We already found some color, then thought we saw the 'end yellow bar,' but now we are seeing more color.
					//This means that the 'yellow' that we previously observed was an anomalous bit of yellow between the endpoints.
					//We will continue to search until we reach the last 'yellow bar.'
				}

				inColor = true;
			}
		}

		if (x1 == x2 && y1 == y2)
			break;

		int errorCopy = error;
		if (errorCopy > -dx)
		{
			error -= dy;
			x1 += sx;
		}

		if (errorCopy < dy)
		{
			error += dx;
			y1 += sy;
		}

		lastX = x1;
		lastY = y1;
	}

}

__forceinline int _get_distance(int x0, int y0, int x1, int y1)
{
	return (int)sqrt(((x1 - x0) * (x1 - x0)) + ((y1 - y0) * (y1 - y0)));
}

///<summary>Reads a <see cref="BarCodeAppearance"/> from an image.</summary>
///<param name="rgba8">The image's pixels, stored in RGBA 8-bit format.</param>
///<param name="width">The width of the image, measured in pixels.</param>
///<param name="height">The height of the image, measured in pixels.</param>
///<param name="sectionCount">The number of sections (value colors) on the bar code.</param>
///<param name="startBox">The <see cref="YellowBoundingBox"/> that surrounds the first 'yellow bar.'</param>
///<param name="endBox">The <see cref="YellowBoundingBox"/> that surrounds the second 'yellow bar.'</param>
///<param name="startX">The X position where the colorful line begins.</param>
///<param name="startY">The Y position where the colorful line begins.</param>
///<param name="endX">The X position where the colorful line ends.</param>
///<param name="endY">The Y position where the colorful line ends.</param>
///<returns>The resulting <see cref="BarCodeAppearance"/>.</returns>
BarCodeAppearance _read_bar_code_appearance(const uint8_t * rgba8, int width, int height, int sectionCount, YellowBoundingBox startBox, YellowBoundingBox endBox, int startX, int startY, int endX, int endY)
{
	BarCodeAppearance ret;
	ret._firstBox = startBox;
	ret._secondBox = endBox;
	ret.sectionCount = sectionCount;
	ret.colorStartX = startX;
	ret.colorStartY = startY;
	ret.colorEndX = endX;
	ret.colorEndY = endY;
	for (int i = 0; i < sectionCount; i++)
	{
		ret.redAverage[i] = 0.0f;
		ret.blueAverage[i] = 0.0f;
		ret.greenAverage[i] = 0.0f;
		ret.pixelCount[i] = 0;
	}

	int origX = startX;
	int origY = startY;
	int dx = abs(endX - startX);
	int dy = abs(endY - startY);
	int sx = startX < endX ? 1 : -1;
	int sy = startY < endY ? 1 : -1;
	int error = (dx > dy ? dx : -dy) / 2;

	int totalDistance = _get_distance(startX, startY, endX, endY);

	while (true) {

		int currentDistance = _get_distance(startX, startY, origX, origY);
		float progress = currentDistance / (float)totalDistance;
		int sectionIndex = (int)(progress * sectionCount);

		//Saturate barIndex (though we should never have to!)
		if (sectionIndex >= BAR_CODE_MAX_COLOR_COUNT)
			sectionIndex = BAR_CODE_MAX_COLOR_COUNT - 1;
		if (sectionIndex < 0)
			sectionIndex = 0;

		uint8_t r = rgba8[((startX + (startY * width)) * 4) + 0];
		uint8_t g = rgba8[((startX + (startY * width)) * 4) + 1];
		uint8_t b = rgba8[((startX + (startY * width)) * 4) + 2];

		float redNess = quantify_red(r, g, b);
		float greenNess = quantify_green(r, g, b);
		float blueNess = quantify_blue(r, g, b);

		ret.redAverage[sectionIndex] += redNess;
		ret.greenAverage[sectionIndex] += greenNess;
		ret.blueAverage[sectionIndex] += blueNess;
		ret.pixelCount[sectionIndex]++;

		if (startX == endX && startY == endY)
			break;

		int errorCopy = error;
		if (errorCopy > -dx)
		{
			error -= dy;
			startX += sx;
		}

		if (errorCopy < dy)
		{
			error += dx;
			startY += sy;
		}
	}

	for (int i = 0; i < sectionCount; i++)
	{
		//Up to here, the 'average' values have actually been sums. Now divide to make them average.
		ret.redAverage[i] /= ret.pixelCount[i];
		ret.greenAverage[i] /= ret.pixelCount[i];
		ret.blueAverage[i] /= ret.pixelCount[i];
	}

	return ret;
}

///<summary>Finds all <see cref="BarCodeAppearance"/>s in an image using already-defined <see cref="YellowBoundingBox"/> regions.</summary>
///<param name="rgba8">The image's pixels, stored in RGBA 8-bit format.</param>
///<param name="width">The width of the image, measured in pixels.</param>
///<param name="height">The height of the image, measured in pixels.</param>
///<param name="yellowCfg">The <see cref="YellowConfig"/> that defines when a pixel is considered 'yellow.'</param>
///<param name="yellowBoxes">The <see cref="YellowBoundingBox"/>es that surround the ends of each bar code, stored in no particular order.
///These are essentially <see cref="YellowBoundingBox"/>es surrounding all yellow regions in the image.</param>
///<param name="yellowBoxCount">The number of <see cref="YellowBoundingBox"/>es in <paramref name="yellowBoxes"/>.</param>
///<param name="sectionCount">The number of sections (value colors) in each bar code.</param>
///<param name="dst">The destination <see cref="BarCodeAppearance"/> buffer.</param>
///<param name="maxCount">The maximum number of <see cref="BarCodeAppearance"/>s to find.</param>
///<returns>The number of <see cref="BarCodeAppearance"/>s that have been found.</returns>
size_t find_bar_code_appearances(const uint8_t * rgba8, int width, int height, YellowConfig yellowCfg, const YellowBoundingBox * yellowBoxes, size_t yellowBoxCount, int sectionCount, BarCodeAppearance * dst, size_t maxCount)
{
	size_t count = 0;

	for (size_t i = 0; i < yellowBoxCount; i++)
	{
		for (size_t j = i + 1; j < yellowBoxCount; j++)
		{
			YellowBoundingBox start = yellowBoxes[i];
			int startX = (start.left + start.right) / 2;
			int startY = (start.top + start.bottom) / 2;

			YellowBoundingBox end = yellowBoxes[j];
			int endX = (end.left + end.right) / 2;
			int endY = (end.top + end.bottom) / 2;
			
			//'start' and 'end' points are inside the yellow bar regions. We want a line that defines the colorful region between the yellow bars.
			//So find the colorful line's endpoints
			_find_colorful_line_endpoints(rgba8, width, height, yellowCfg, &startX, &startY, &endX, &endY);

			if (startX == endX && startY == endY)
				continue;//Cannot scan a line with no length, so skip it

			if (count < maxCount)
				dst[count++] = _read_bar_code_appearance(rgba8, width, height, sectionCount, start, end, startX, startY, endX, endY);
			else
				return count;
		}
	}

	return count;
}

///<summary>Searches through a set of <see cref="BarCodeAppearance"/>s to find those that match a specific <see cref="BarCode"/>.</summary>
///<param name="barCode">The specific <see cref="BarCode"/> to find.</param>
///<param name="minLineDistance">The minimum length of the 'color' line. All <see cref="BarCodeAppearance"/>s with a 'color' line shorter than
///this length will be ignored.</param>
///<param name="minMatchScore">The minimum 'match' score of a <see cref="BarCodeAppearance"/> compared to the <paramref name="barCode"/>,
///as determined by <see cref="quantify_bar_code_appearance_match(const BarCode*, const BarCodeAppearance*)"/>.</param>
///<param name="appearances">All <see cref="BarCodeAppearance"/>s.</param>
///<param name="appearanceCount">The number of <paramref name="appearances"/>.</param>
///<param name="results">Will contain pointers to the <see cref="BarCodeAppearance"/>s that matched the <paramref name="barCode"/>.</param>
///<param name="resultScores">Will contain the 'match' score of each <see cref="BarCodeAppearance"/> in <paramref name="results"/>, as determined by
///the <see cref="quantify_bar_code_appearance_match(const BarCode*, const BarCodeAppearance*)"/> function.</param>
///<param name="maxResultCount">The maximum number of values that can be stored in <paramref name="results"/> and <paramref name="resultScores"/>.</param>
///<returns>The number of <see cref="BarCodeAppearances"/> that matched the <paramref name="barCode"/>.</returns>
size_t find_appearances_of_bar_code(BarCode barCode, int minLineDistance, float minMatchScore, const BarCodeAppearance * appearances, size_t appearanceCount, const BarCodeAppearance * *results, float* resultScores, size_t maxResultCount)
{
	size_t count = 0;
	for (size_t i = 0; i < appearanceCount; i++)
	{
		int lineDistance = _get_distance(appearances[i].colorStartX, appearances[i].colorStartY, appearances[i].colorEndX, appearances[i].colorEndY);
		if (lineDistance < minLineDistance)
			continue;//This appearance's line is considered too small

		float matchScore = quantify_bar_code_appearance_match(&barCode, &appearances[i]);
		if (matchScore < minMatchScore)
			continue;//This appearance doesn't meet the minimum match score

		//Find where to insert the appearance in the sorted array
		size_t insertPosition;
		bool foundInsertPosition = false;
		for (size_t j = 0; j < count; j++)
		{
			if (matchScore > resultScores[j])
			{
				foundInsertPosition = true;
				insertPosition = j;
				break;
			}
		}

		if (!foundInsertPosition)
		{
			if (count < maxResultCount)
			{
				//This appearance does not have a higher 'match' score than any others so far, so we will put this one at the end
				insertPosition = count;//We will increment 'count' later
			}
			else
			{
				//We can't store a record of this appearance since the buffer is full.
				//But since 'foundInsertPosition' is false, we know that this appearance had a low score, 
				//so let's just ignore it (and continue searching for more appearances which may have a higher score)
				continue;
			}
		}

		if (insertPosition < maxResultCount && count < maxResultCount)
		{
			if (count > 0)
			{
				//First move every value at and after 'insertPosition' down by one
				for (size_t j = count; j > insertPosition; j--)
				{
					results[j] = results[j - 1];
					resultScores[j] = resultScores[j - 1];
				}
			}

			results[insertPosition] = &appearances[i];
			resultScores[insertPosition] = matchScore;
			count++;
		}
	}

	return count;
}

///<summary>Structure that stores a 'BarCode find request' and all of its <see cref="BarCodeAppearance"/>s.</summary>
///<remarks>This structure is used for the <see cref="find_appearances_of_bar_code_interests_in_bitmap"/> function to store the
///input <see cref="BarCode"/> 'request' as well as all of the <see cref="BarCodeAppearance"/>s that were found to match
///the 'request'. You may reuse a <see cref="BarCodeFindContext"/> for repeated calls to <see cref="find_appearances_of_bar_code_interests_in_bitmap"/>.
///The <see cref="appearanceBuffer"/> stores the <see cref="BarCodeAppearance"/>s that were found in the most recent call
///to <see cref="find_appearances_of_bar_code_interests_in_bitmap"/>.</remarks>
typedef struct BarCodeFindContext
{
	///<summary>The <see cref="BarCodeAppearance"/> buffer which will store the <see cref="BarCodeAppearance"/>s, sorted
	///in descending match score order (determined by <see cref="quantify_bar_code_appearance_match(const BarCode*, const BarCodeAppearance*)"/>).</summary>
	///<remarks>The <see cref="find_appearances_of_bar_code_interests_in_bitmap"/> function will fill this buffer. The number of values is defined
	///by <see cref="appearanceCount"/>, and the capacity is defined by <see cref="appearanceBufferCapacity"/>.</remarks>
	BarCodeAppearance* appearanceBuffer;

	///<summary>Buffer that stores the 'match score' of each <see cref="BarCodeAppearance"/> in <see cref="appearanceBuffer"/>, as
	///determined by the <see cref="quantify_bar_code_appearance_match"/> function.</summary>
	///<remarks>Each value in this buffer corresponds to the 'match score' of the <see cref="BarCodeAppearance"/> at the same position
	///in <see cref="appearanceBuffer"/>. This buffer must have the same capacity as <see cref="appearanceBuffer"/>, which is defined
	///by <see cref="appearanceBufferCapacity"/>.</remarks>
	float* appearanceMatchScores;

	///<summary>The number of <see cref="BarCodeAppearance"/>s that can fit in <see cref="appearanceBuffer"/>.</summary>
	size_t appearanceBufferCapacity;

	///<summary>The number of <see cref="BarCodeAppearance"/>s that are currently stored in <see cref="appearanceBuffer"/>.</summary>
	size_t appearanceCount;

	///<summary>The <see cref="BarCode"/> to find.</summary>
	BarCode barCode;

	///<summary>The minimum 'match score' of a <see cref="BarCodeAppearance"/>, as determined by <see cref="quantify_bar_code_appearance_match"/>.</summary>
	///<remarks>The <see cref="find_appearances_of_bar_code_interests_in_bitmap"/> function will not store any <see cref="BarCodeAppearance"/> with a match
	///score that is less than this value in the <see cref="appearanceBuffer"/>.</remarks>
	float minMatchScore;

	///<summary>The minimum length of the colorful portion of the BarCode, measured in pixels.</summary>
	///<remarks>The <see cref="find_appearances_of_bar_code_interests_in_bitmap"/> function will not store any <see cref="BarCodeAppearance"/> if the length of its 'color line' is less than
	///this value.</remarks>
	int minLineDistance;

} BarCodeFindContext;

///<summary>Contains temporary memory for use by the <see cref="find_appearances_of_bar_code_interests_in_bitmap"/> function.</summary>
///<remarks>This structure can (and should) be shared across repeated calls to <see cref="find_appearances_of_bar_code_interests_in_bitmap"/>.</remarks>
typedef struct BarCodeFindTemporaryMemory
{
	///<summary>Array of <see cref="YellowScanLine"/>s.</summary>
	YellowScanLine* scanLines;

	///<summary>The maximum number of <see cref="YellowScanLine"/>s that can be stored in <see cref="scanLines"/>. Generally,
	///this should be somewhere near the number of horizontal lines in the input bitmap(s). If you provide too few, some
	///<see cref="BarCode"/>s may go unnoticed.</summary>
	size_t scanLineCapacity;

	///<summary>Array of <see cref="YellowBoundingBox"/>es.</summary>
	YellowBoundingBox* yellowBoxes;

	///<summary>The maximum number of <see cref="YellowBoundingBox"/>es that can be stored in <see cref="yellowBoxes"/>. Generally,
	///this should be somewhere near twice the maximum number of <see cref="BarCodeAppearance"/>s that you expect to find in a single
	///bitmap (twice due to there being a yellow box at both ends of a BarCode), plus a reasonable padding for 'noise' boxes that will
	///likely be found. If you provide too few, some <see cref="BarCode"/>s may go unnoticed.</summary>
	size_t yellowBoxCapacity;

	///<summary>Array of indices, used for sorting.</summary>
	size_t* temporaryIndexBuffer;

	///<summary>The maximum number of indices that can be stored in <see cref="temporaryIndexBuffer"/>. Generally, this should
	///be slightly greater than the maximum number of <see cref="BarCodeAppearance"/>s you expect to be found in a single
	///bitmap, plus some padding for noise. If you provide too few, some <see cref="BarCode"/>s may go unnoticed.</summary>
	size_t temporaryIndexBufferCapacity;

	///<summary>Buffer that will store all <see cref="BarCodeAppearance"/>s that are found in a bitmap.</summary>
	BarCodeAppearance* appearances;

	///<summary>The maximum number of <see cref="BarCodeAppearance"/>s that can be stored in <see cref="appearances"/>. Generally,
	///this should be slightly greater than the maximum number of <see cref="BarCodeAppearance"/>s that you expect to find in a
	///single bitmap, plus reasonable padding for noise. If you provide too few, some <see cref="BarCode"/>s may go unnoticed.</summary>
	size_t appearanceCapacity;

	///<summary>Array of pointers, used for sorting. The capacity is defined by <see cref="appearanceSortBufferCapacity"/>.</summary>
	BarCodeAppearance** appearanceSortBuffer;

	///<summary>Array of 'match scores', used for sorting. The capacity is defined by <see cref="appearanceSortBufferCapacity"/>.</summary>
	float* appearanceSortMatchScoreBuffer;

	///<summary>The maximum number of <see cref="BarCodeAppearance"/> <em>pointers</em> that can be stored in <see cref="appearanceSortBuffer"/>.
	///This also defines the maximum number of floats that can be stored in <see cref="appearanceSortMatchScoreBuffer"/>. Generally, this should
	///be slightly greater than the maximum number of <see cref="BarCodeAppearance"/>s you expect to find in a single bitmap, plus some padding
	///for noise. If you provide too few, some <see cref="BarCode"/>s may go unnoticed.</summary>
	size_t appearanceSortBufferCapacity;

} BarCodeFindTemporaryMemory;

///<summary>Searches for <see cref="BarCodeAppearance"/>s for a set of <see cref="BarCodeFindContext"/>s.</summary>
///<param name="rgba8">The image's pixels, stored in RGBA 8-bit format.</param>
///<param name="width">The width, in pixels, of the bitmap.</param>
///<param name="height">The height, in pixels, of the bitmap.</param>
///<param name="yellowCfg">The <see cref="YellowConfig"/> that determines when a pixel is considered 'yellow'.</param>
///<param name="maxYellowSpacing">The maximum distance between 'yellow' pixels before they are considered separate 'yellow bounding boxes'.</param>
///<param name="contexts">Array of <see cref="BarCodeFindContext"/>s.</param>
///<param name="contextCount">The number of <see cref="BarCodeFindContext"/>s in <paramref name="contexts"/>.</param>
///<param name="memory">The <see cref="BarCodeFindTemporaryMemory"/> that provides temporary memory for this function.</param>
///<remarks>All <see cref="BarCodeFindContext"/>s must have a <see cref="BarCode"/> with the same number of 'sections' (see <see cref="BarCode.colorCount"/>).</remarks>
void find_appearances_of_bar_code_interests_in_bitmap(const uint8_t* rgba8, int width, int height, YellowConfig yellowCfg, int maxYellowSpacing, BarCodeFindContext* contexts, size_t contextCount, BarCodeFindTemporaryMemory memory)
{
	if (contextCount == 0)
		return;//Nothing to do

	//Determine the number of 'sections' on all BarCodes (this function requires the same number of sections per BarCode)
	int sectionCount = 0;
	for (size_t i = 0; i < contextCount; i++)
	{
		if (i == 0)
			sectionCount = (int)contexts[i].barCode.colorCount;
		else
			assert(sectionCount == contexts[i].barCode.colorCount);//Make sure all BarCodes have the same 'section count' (color count)
	}

	//Find the 'yellow scan lines'
	size_t scanLineCount = find_yellow_lines(rgba8, width, height, yellowCfg, memory.scanLines, memory.scanLineCapacity);

	//Find the 'yellow bounding boxes'
	size_t boxCount = find_yellow_rectangles(memory.scanLines, scanLineCount, maxYellowSpacing, memory.temporaryIndexBuffer, memory.temporaryIndexBufferCapacity, memory.yellowBoxes, memory.yellowBoxCapacity);

	//Find all BarCodeAppearances
	size_t appearanceCount = find_bar_code_appearances(rgba8, width, height, yellowCfg, memory.yellowBoxes, boxCount, sectionCount, memory.appearances, memory.appearanceCapacity);

	//Match all BarCodeAppearances to their respective BarCodeFindContext (if any)
	for (size_t i = 0; i < contextCount; i++)
	{
		//Sort all BarCodeAppearances by how well they match the context's BarCode
		contexts[i].appearanceCount = find_appearances_of_bar_code(contexts[i].barCode, contexts[i].minLineDistance, contexts[i].minMatchScore, memory.appearances, appearanceCount, memory.appearanceSortBuffer, memory.appearanceSortMatchScoreBuffer, memory.appearanceSortBufferCapacity);
		if (contexts[i].appearanceCount > contexts[i].appearanceBufferCapacity)
			contexts[i].appearanceCount = contexts[i].appearanceBufferCapacity;

		//Copy the BarCodeAppearances to the context's destination buffer
		for (size_t j = 0; j < contexts[i].appearanceCount; j++)
		{
			BarCodeAppearance appearance = *(memory.appearanceSortBuffer[j]);
			contexts[i].appearanceBuffer[j] = appearance;
			contexts[i].appearanceMatchScores[j] = memory.appearanceSortMatchScoreBuffer[j];
		}
	}
}