using System.Drawing;

namespace BarCodeFinder
{
    public struct BarCodeAppearance
    {
        public Rectangle FirstBox { get; private set; }

        public Rectangle SecondBox { get; private set; }

        public Point ColorStart { get; private set; }

        public Point ColorEnd { get; private set; }

        public float MatchScore { get; private set; }

        public BarCodeAppearance(Rectangle firstBox, Rectangle secondBox, Point colorStart, Point colorEnd, float matchScore)
        {
            this.FirstBox = firstBox;
            this.SecondBox = secondBox;
            this.ColorStart = colorStart;
            this.ColorEnd = colorEnd;
            this.MatchScore = matchScore;
        }
    }
}
