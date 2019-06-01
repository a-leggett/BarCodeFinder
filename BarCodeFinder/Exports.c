#include "BarCode.h"

_declspec(dllexport) void ShowYellow(const uint8_t* rgba8Source, uint8_t* rgba8Dest, int width, int height, YellowConfig config, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	show_yellow(rgba8Source, rgba8Dest, width, height, config, r, g, b, a);
}

_declspec(dllexport) BarCodeFindTemporaryMemory* AllocateBarCodeFindTemporaryMemory(size_t scanLineCapacity, size_t yellowBoxCapacity, size_t tempIndexBufferCapacity, size_t appearanceCapacity, size_t appearanceSortBufferCapacity)
{
	BarCodeFindTemporaryMemory* ret = (BarCodeFindTemporaryMemory*)malloc(sizeof(BarCodeFindTemporaryMemory));
	if (ret == NULL)
		return NULL;

	YellowScanLine* scanLines = (YellowScanLine*)malloc(sizeof(YellowScanLine) * scanLineCapacity);
	YellowBoundingBox* boxes = (YellowBoundingBox*)malloc(sizeof(YellowBoundingBox) * yellowBoxCapacity);
	size_t* tempIndexBuf = (size_t*)malloc(sizeof(size_t) * tempIndexBufferCapacity);
	BarCodeAppearance* appearances = (BarCodeAppearance*)malloc(sizeof(BarCodeAppearance) * appearanceCapacity);
	BarCodeAppearance** appearanceSortBuffer = (BarCodeAppearance * *)malloc(sizeof(BarCodeAppearance*) * appearanceSortBufferCapacity);
	float* appearanceSortMatchScoreBuffer = (float*)malloc(sizeof(float) * appearanceSortBufferCapacity);

	if (scanLines == NULL || boxes == NULL || tempIndexBuf == NULL || appearances == NULL || appearanceSortBuffer == NULL || appearanceSortMatchScoreBuffer == NULL)
	{
		//An allocation failed, so free all allocations that did not fail
		if (scanLines != NULL)
			free(scanLines);
		if (boxes != NULL)
			free(boxes);
		if (tempIndexBuf != NULL)
			free(tempIndexBuf);
		if (appearances != NULL)
			free(appearances);
		if (appearanceSortBuffer != NULL)
			free(appearanceSortBuffer);
		if (appearanceSortMatchScoreBuffer != NULL)
			free(appearanceSortMatchScoreBuffer);

		free(ret);
		return NULL;
	}
	else
	{
		ret->scanLines = scanLines;
		ret->scanLineCapacity = scanLineCapacity;
		
		ret->yellowBoxes = boxes;
		ret->yellowBoxCapacity = yellowBoxCapacity;

		ret->temporaryIndexBuffer = tempIndexBuf;
		ret->temporaryIndexBufferCapacity = tempIndexBufferCapacity;

		ret->appearances = appearances;
		ret->appearanceCapacity = appearanceCapacity;

		ret->appearanceSortBuffer = appearanceSortBuffer;
		ret->appearanceSortBufferCapacity = appearanceSortBufferCapacity;
		ret->appearanceSortMatchScoreBuffer = appearanceSortMatchScoreBuffer;

		return ret;
	}
}

_declspec(dllexport) FreeBarCodeFindTemporaryMemory(BarCodeFindTemporaryMemory* memory)
{
	free(memory->scanLines);
	free(memory->yellowBoxes);
	free(memory->temporaryIndexBuffer);
	free(memory->appearances);
	free(memory->appearanceSortBuffer);
	free(memory->appearanceSortMatchScoreBuffer);
}

_declspec(dllexport) BarCodeFindContext* AllocateBarCodeFindContextArray(size_t count)
{
	return (BarCodeFindContext*)malloc(sizeof(BarCodeFindContext) * count);
}

_declspec(dllexport) void FreeBarCodeFindContextArray(BarCodeFindContext* array)
{
	free(array);
}

_declspec(dllexport) bool TryInitBarCodeFindContext(BarCodeFindContext* array, size_t index, size_t appearanceBufferCapacity, int barCodeColorCount, BarCodeColor* barCodeColors, float minMatchScore, int minLineDistance)
{
	BarCodeAppearance* appearanceBuf = (BarCodeAppearance*)malloc(sizeof(BarCodeAppearance) * appearanceBufferCapacity);
	if (appearanceBuf == NULL)
		return false;
	float* appearanceMatchScores = (float*)malloc(sizeof(float) * appearanceBufferCapacity);
	if (appearanceMatchScores == NULL)
	{
		free(appearanceBuf);
		return false;
	}

	array[index].appearanceBuffer = appearanceBuf;
	array[index].appearanceBufferCapacity = appearanceBufferCapacity;
	array[index].appearanceCount = 0;
	array[index].appearanceMatchScores = appearanceMatchScores;
	
	array[index].barCode.colorCount = barCodeColorCount;
	for(int i = 0; i < array[index].barCode.colorCount; i++)
		array[index].barCode.colors[i] = barCodeColors[i];

	array[index].minLineDistance = minLineDistance;
	array[index].minMatchScore = minMatchScore;
	return true;
}

_declspec(dllexport) int GetBarCodeAppearanceCount(BarCodeFindContext* contextArray, size_t index)
{
	return (int)contextArray[index].appearanceCount;
}

_declspec(dllexport) bool TryReadBarCodeAppearance(BarCodeFindContext* contextArray, size_t contextIndex, size_t appearanceIndex, int* points, float* matchScore)
{
	if (appearanceIndex >= contextArray[contextIndex].appearanceCount)
		return false;
	BarCodeAppearance appearance = contextArray[contextIndex].appearanceBuffer[appearanceIndex];

	points[0] = appearance.colorStartX;
	points[1] = appearance.colorStartY;

	points[2] = appearance.colorEndX;
	points[3] = appearance.colorEndY;

	points[4] = appearance._firstBox.left;
	points[5] = appearance._firstBox.top;
	points[6] = appearance._firstBox.right;
	points[7] = appearance._firstBox.bottom;

	points[8] = appearance._secondBox.left;
	points[9] = appearance._secondBox.top;
	points[10] = appearance._secondBox.right;
	points[11] = appearance._secondBox.bottom;

	matchScore[0] = quantify_bar_code_appearance_match(&contextArray[contextIndex].barCode, &appearance);

	return true;
}

_declspec(dllexport) void ReleaseBarCodeFindContext(BarCodeFindContext* array, size_t index)
{
	array[index].appearanceBufferCapacity = 0;
	free(array[index].appearanceBuffer);
	array[index].appearanceBuffer = NULL;
}

_declspec(dllexport) void FindAppearancesOfBarCodeInterestsInBitmap(const uint8_t* rgba8, int width, int height, YellowConfig yellowCfg, int maxYellowSpacing, BarCodeFindContext* contexts, size_t contextCount, BarCodeFindTemporaryMemory* memory)
{
	find_appearances_of_bar_code_interests_in_bitmap(rgba8, width, height, yellowCfg, maxYellowSpacing, contexts, contextCount, *memory);
}

_declspec(dllexport) void ConvertFromBGRAToRGBA(const uint8_t* src, uint8_t* dst, int width, int height)
{
	for (int i = 0; i < width * height * 4; i += 4)
	{
		uint8_t b = src[i + 0];
		uint8_t g = src[i + 1];
		uint8_t r = src[i + 2];
		uint8_t a = src[i + 3];

		dst[i + 0] = r;
		dst[i + 1] = g;
		dst[i + 2] = b;
		dst[i + 3] = a;
	}
}

_declspec(dllexport) void ConvertFromRGBAToBGRA(uint8_t* src, uint8_t* dst, int width, int height)
{
	for (int i = 0; i < width * height * 4; i += 4)
	{
		uint8_t r = src[i + 0];
		uint8_t g = src[i + 1];
		uint8_t b = src[i + 2];
		uint8_t a = src[i + 3];

		dst[i + 0] = b;
		dst[i + 1] = g;
		dst[i + 2] = r;
		dst[i + 3] = a;
	}
}