using BarCodeFinder.Native;
using System;

namespace BarCodeFinder
{
    public sealed class BarCodeFinder : IDisposable
    {
        /// <summary>
        /// Pointer to the native 'BarCodeFindTemporaryMemory' structure.
        /// </summary>
        private IntPtr barCodeFindTemporaryMemory;

        public BarCodeFinder(uint scanLineCapacity = 1080 * 16, uint yellowBoxCapacity = 512 * 16, uint tempIndexCapacity = 512 * 16, uint appearanceCapacity = 512 * 16, uint appearanceSortBufferCapacity = 512)
        {
            IntPtr pointer = Imports.AllocateBarCodeFindTemporaryMemory(scanLineCapacity, yellowBoxCapacity, tempIndexCapacity, appearanceCapacity, appearanceSortBufferCapacity);
            if (pointer == IntPtr.Zero)
                throw new InvalidOperationException("Failed to allocate the native memory. Perhaps an argument was too large.");
            this.barCodeFindTemporaryMemory = pointer;
        }

        public void Find(IntPtr rgba8, int width, int height, YellowConfig yellowConfig, BarCodeFindContextArray array, int maxYellowSpacing = 5)
        {
            Imports.FindAppearancesOfBarCodeInterestsInBitmap(rgba8, width, height, yellowConfig, maxYellowSpacing, array.nativePointer, (ulong)array.Count, this.barCodeFindTemporaryMemory);
        }

        #region IDisposable Support
        public bool IsDisposed { get; private set; } = false; // To detect redundant calls

        void Dispose(bool disposing)
        {
            if (!IsDisposed)
            {
                if (disposing)
                {
                    if(barCodeFindTemporaryMemory != IntPtr.Zero)
                        Imports.FreeBarCodeFindTemporaryMemory(barCodeFindTemporaryMemory);
                }

                barCodeFindTemporaryMemory = IntPtr.Zero;
                IsDisposed = true;
            }
        }

        ~BarCodeFinder()
        {
          Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
