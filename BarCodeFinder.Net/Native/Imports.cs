using System;
using System.Runtime.InteropServices;

namespace BarCodeFinder.Native
{
    internal static class Imports
    {
        public const string Filename = "BarCodeFinder.dll";

        [DllImport(Filename)]
        public static extern void ShowYellow(IntPtr rgba8Source, IntPtr rgba8Dest, int width, int height, YellowConfig config, byte r, byte g, byte b, byte a);

        [DllImport(Filename)]
        public static extern IntPtr AllocateBarCodeFindTemporaryMemory(ulong scanLineCapacity, ulong yellowBoxCapacity, ulong tempIndexBufferCapacity, ulong appearanceCapacity, ulong appearanceSortBufferCapacity);

        [DllImport(Filename)]
        public static extern void FreeBarCodeFindTemporaryMemory(IntPtr pointer);

        [DllImport(Filename)]
        public static extern IntPtr AllocateBarCodeFindContextArray(ulong count);

        [DllImport(Filename)]
        public static extern void FreeBarCodeFindContextArray(IntPtr pointer);

        [DllImport(Filename)]
        public static extern bool TryInitBarCodeFindContext(IntPtr contextArrayPointer, ulong contextIndex, ulong appearanceCapacity, int barCodeColorCount, [In] int[] barCodeColors, float minMatchScore, int minLineDistance);

        [DllImport(Filename)]
        public static extern int GetBarCodeAppearanceCount(IntPtr contextArrayPointer, ulong contextIndex);

        [DllImport(Filename)]
        public static extern bool TryReadBarCodeAppearance(IntPtr contextArrayPointer, ulong contextIndex, ulong appearanceIndex, [Out] int[] points, [Out] float[] matchScore);

        [DllImport(Filename)]
        public static extern void ReleaseBarCodeFindContext(IntPtr contextArrayPointer, ulong index);

        [DllImport(Filename)]
        public static extern void FindAppearancesOfBarCodeInterestsInBitmap(IntPtr rgba8, int width, int height, YellowConfig yellowConfig, int maxYellowSpacing, IntPtr barCodeFindContextArray, ulong barCodeFindContextArrayCount, IntPtr barCodeFindTemporaryMemory);

        [DllImport(Filename)]
        public static extern void ConvertFromBGRAToRGBA(IntPtr src, IntPtr dst, int width, int height);

        [DllImport(Filename)]
        public static extern void ConvertFromRGBAToBGRA(IntPtr src, IntPtr dst, int width, int height);
    }
}
