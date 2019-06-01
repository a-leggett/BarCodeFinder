using BarCodeFinder.Native;
using System;
using System.Drawing;
using System.Runtime.InteropServices;

namespace BarCodeFinder
{
    [StructLayout(LayoutKind.Sequential)]
    public struct YellowConfig
    {
        public byte maxRedGreenSeparation;

        public byte minRedBlueSeparation;

        public byte minRed;

        public void Show(IntPtr rgba8, int width, int height, Color color)
        {
            Imports.ShowYellow(rgba8, rgba8, width, height, this, color.R, color.G, color.B, color.A);
        }
    }
}
