using BarCodeFinder.Native;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;

namespace BarCodeFinder
{
    public sealed class BarCodeFindContext : IReadOnlyList<BarCodeAppearance>
    {
        private const string DisposedMessage = "The " + nameof(BarCodeFindContextArray) + " to which this " + nameof(BarCodeFindContext) + " belongs has been disposed.";
        private WeakReference<BarCodeFindContextArray> array;

        public BarCode BarCode { get; private set; }

        internal BarCodeFindContext(BarCodeFindContextArray array, int index, BarCode barCode)
        {
            this.array = new WeakReference<BarCodeFindContextArray>(array);
            this.Index = index;
            this.BarCode = barCode;
        }

        public BarCodeAppearance this[int index]
        {
            get
            {
                if (index < 0)
                    throw new ArgumentOutOfRangeException(nameof(index));

                var arr = this.Array;
                if(arr != null)
                {
                    int[] points = new int[12];
                    float[] score = new float[1];
                    if(Imports.TryReadBarCodeAppearance(arr.nativePointer, (ulong)this.Index, (ulong)index, points, score))
                    {
                        int colorStartX = points[0];
                        int colorStartY = points[1];

                        int colorEndX = points[2];
                        int colorEndY = points[3];

                        int firstBoxLeft = points[4];
                        int firstBoxTop = points[5];
                        int firstBoxRight = points[6];
                        int firstBoxBottom = points[7];

                        int secondBoxLeft = points[8];
                        int secondBoxTop = points[9];
                        int secondBoxRight = points[10];
                        int secondBoxBottom = points[11];

                        return new BarCodeAppearance(new Rectangle(firstBoxLeft, firstBoxTop, firstBoxRight - firstBoxLeft, firstBoxBottom - firstBoxTop), new Rectangle(secondBoxLeft, secondBoxTop, secondBoxRight - secondBoxLeft, secondBoxBottom - secondBoxTop), new Point(colorStartX, colorStartY), new Point(colorEndX, colorEndY), score[0]);
                    }
                    else
                    {
                        throw new ArgumentOutOfRangeException(nameof(index));
                    }
                }
                else
                {
                    throw new ObjectDisposedException(nameof(BarCodeFindContextArray), DisposedMessage);
                }
            }
        }

        /// <summary>
        /// The <see cref="BarCodeFindContextArray"/> to which this <see cref="BarCodeFindContext"/> belongs,
        /// or null if it has been disposed.
        /// </summary>
        public BarCodeFindContextArray Array
        {
            get
            {
                if (array.TryGetTarget(out var ret))
                {
                    if (!ret.IsDisposed)
                        return ret;
                    else
                        return null;
                }
                else
                {
                    return null;
                }
            }
        }

        /// <summary>
        /// The index of this <see cref="BarCodeFindContext"/> on the <see cref="Array"/>.
        /// </summary>
        public int Index { get; private set; }

        public int Count
        {
            get
            {
                var arr = this.Array;
                if (arr != null)
                    return Imports.GetBarCodeAppearanceCount(arr.nativePointer, (ulong)this.Index);
                else
                    throw new ObjectDisposedException(nameof(BarCodeFindContextArray), DisposedMessage);
            }
        }

        public IEnumerator<BarCodeAppearance> GetEnumerator()
        {
            var arr = this.Array;
            if (arr != null)
            {
                for (int i = 0; i < Count; i++)
                    yield return this[i];
            }
            else
            {
                throw new ObjectDisposedException(nameof(BarCodeFindContextArray), DisposedMessage);
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
