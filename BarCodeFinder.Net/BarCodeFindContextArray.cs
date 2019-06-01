using BarCodeFinder.Native;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace BarCodeFinder
{
    public sealed class BarCodeFindContextArray : IDisposable, IReadOnlyList<BarCodeFindContext>
    {
        private List<BarCodeFindContext> contexts = new List<BarCodeFindContext>(16);
        internal IntPtr nativePointer;

        public BarCodeFindContextArray(IReadOnlyCollection<BarCode> barCodes, float minMatchScore, int minLineDistance = 8, int appearanceCapacityPerBarCode = 16)
        {
            if (barCodes == null)
                throw new ArgumentNullException(nameof(barCodes));

            //Make sure all BarCodes have the same number of colors (section count)
            int colorCount = -1;
            foreach(var code in barCodes)
            {
                if (colorCount == -1)
                {
                    colorCount = code.Count;
                }
                else
                {
                    if (code.Count != colorCount)
                        throw new ArgumentException("All of the "+nameof(BarCode)+"s must have the same number of colors (aka section count).", nameof(barCodes));
                }
            }

            //Allocate the entire array
            nativePointer = Imports.AllocateBarCodeFindContextArray((ulong)barCodes.Count);
            if(nativePointer != null)
            {
                //Allocate each context
                int index = 0;
                foreach(var code in barCodes)
                {
                    BarCodeFindContext context = new BarCodeFindContext(this, index, code);
                    contexts.Add(context);
                    if(!Imports.TryInitBarCodeFindContext(nativePointer, (ulong)index, (ulong)appearanceCapacityPerBarCode, code.Count, code.Select(x=>(int)x).ToArray(), minMatchScore, minLineDistance))
                    {
                        //Oh no, free all previous ones
                        for (int i = 0; i < index; i++)
                            Imports.ReleaseBarCodeFindContext(nativePointer, (ulong)i);

                        Imports.FreeBarCodeFindContextArray(nativePointer);
                        throw new InvalidOperationException("Failed to allocate the required native resources. Perhaps an argument was too large.");
                    }

                    index++;
                }
            }
            else
            {
                throw new InvalidOperationException("Failed to allocate native memory. Perhaps an argument was too large.");
            }
        }

        #region IDisposable Support
        public bool IsDisposed { get; private set; } = false;

        public int Count
        {
            get
            {
                if (IsDisposed)
                    throw new ObjectDisposedException(nameof(BarCodeFindContextArray));

                return contexts.Count;
            }
        }

        public BarCodeFindContext this[int index]
        {
            get
            {
                if (IsDisposed)
                    throw new ObjectDisposedException(nameof(BarCodeFindContextArray));

                return contexts[index];
            }
        }

        void Dispose(bool disposing)
        {
            if (!IsDisposed)
            {
                if (disposing)
                {
                    if(nativePointer != IntPtr.Zero)
                    {
                        //First release all of the contexts
                        for (int i = 0; i < Count; i++)
                            Imports.ReleaseBarCodeFindContext(nativePointer, (ulong)i);

                        //Now free the array
                        Imports.FreeBarCodeFindContextArray(nativePointer);
                    }
                }

                nativePointer = IntPtr.Zero;
                IsDisposed = true;
            }
        }

        ~BarCodeFindContextArray()
        {
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        public IEnumerator<BarCodeFindContext> GetEnumerator()
        {
            if (IsDisposed)
                throw new ObjectDisposedException(nameof(BarCodeFindContextArray));

            return contexts.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return contexts.GetEnumerator();
        }
        #endregion
    }
}
