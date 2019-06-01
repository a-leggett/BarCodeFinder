using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace BarCodeFinder
{
    public struct BarCode : IReadOnlyList<BarCodeColor>
    {
        public const int MaxColorCount = 25;

        private BarCodeColor[] colors;

        public BarCodeColor this[int index]
        {
            get
            {
                if (index < 0 || index >= colors.Length)
                    throw new ArgumentOutOfRangeException(nameof(index));
                return colors[index];
            }
        }

        public int Count { get { return colors.Length; } }

        public IEnumerator<BarCodeColor> GetEnumerator()
        {
            foreach (var color in colors)
                yield return color;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return colors.GetEnumerator();
        }

        public BarCode(IReadOnlyCollection<BarCodeColor> colors)
        {
            if (colors == null)
                throw new ArgumentNullException(nameof(colors));
            if (colors.Count > MaxColorCount)
                throw new ArgumentException("There cannot be more than " + nameof(MaxColorCount) + " colors in a " + nameof(BarCode) + ".", nameof(colors));

            this.colors = colors.ToArray();
        }

        public override bool Equals(object obj)
        {
            if (obj is BarCode other)
            {
                if (this.colors == null || other.colors == null)
                {
                    return this.colors == other.colors;
                }
                else
                {
                    if (this.colors.Length == other.colors.Length)
                    {
                        for (int i = 0; i < this.colors.Length; i++)
                        {
                            if (this.colors[i] != other.colors[i])
                                return false;
                        }

                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            if (this.colors == null)
                return 0;

            int ret = 14741;
            foreach (var color in this)
                ret = ret * 727 + color.GetHashCode();

            return ret;
        }
    }
}
