using BarCodeFinder;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;

namespace BarCodeFinderDemo
{
    class Program
    {
        private static BarCode ParseBarCode(string colors)
        {
            List<BarCodeColor> ret = new List<BarCodeColor>(25);
            foreach(var c in colors)
            {
                BarCodeColor currentColor;
                switch (c)
                {
                    case 'R':
                    case 'r':
                        currentColor = BarCodeColor.Red;
                        break;

                    case 'G':
                    case 'g':
                        currentColor = BarCodeColor.Green;
                        break;

                    case 'B':
                    case 'b':
                        currentColor = BarCodeColor.Blue;
                        break;

                    default:
                        throw new FormatException("Unrecognized color character: '"+c+"', expected 'R', 'G', or 'B'.");
                }

                ret.Add(currentColor);
            }
            if (ret.Count > BarCode.MaxColorCount)
                throw new FormatException("There cannot be more than " + BarCode.MaxColorCount + " color characters in a " + nameof(BarCode) + " string.");

            return new BarCode(ret);
        }

        static void Main(string[] args)
        {
            Console.WriteLine(Environment.CurrentDirectory);
            string path;
            string colorString;

            if (args.Length < 1)
            {
                Console.WriteLine("Enter the path to the source image:");
                path = Console.ReadLine();
            }
            else
            {
                path = args[0];
            }

            if (args.Length < 2)
            {
                Console.WriteLine("Enter the BarCode to find in the image, with each character representing a color\n[For example: RGBGGRBRGB]:");
                colorString = Console.ReadLine();
            }
            else
            {
                colorString = args[1];
            }

            BarCode toFind;
            try
            {
                toFind = ParseBarCode(colorString);
            }
            catch(FormatException fe)
            {
                Console.WriteLine("Failed to parse the BarCode string due to a FormatException:\n" + fe);
                Environment.Exit(-1);
            }

            //TODO: Tune this YellowConfig for your environment
            YellowConfig yellowConfig = new YellowConfig() { maxRedGreenSeparation = 45, minRedBlueSeparation = 50, minRed = 170 };
            BarCodeFinder.BarCodeFinder finder = new BarCodeFinder.BarCodeFinder();
            BarCodeFindContextArray arr = new BarCodeFindContextArray(new BarCode[] { toFind }, 0.1f, 8, 6400);

            //Load the image
            Bitmap srcImg = new Bitmap(path);

            //Get its pixels
            var data = srcImg.LockBits(new Rectangle(0, 0, srcImg.Width, srcImg.Height), ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
            
            //Convert to RGBA (from BGRA - since we are on little endian)
            BitmapHelper.ConvertFromBGRAToRGBA(data.Scan0, data.Scan0, data.Width, data.Height);

            //Find the BarCodeAppearances
            finder.Find(data.Scan0, data.Width, data.Height, yellowConfig, arr, 2);
            yellowConfig.Show(data.Scan0, data.Width, data.Height, Color.Green);

            //Convert back to BGRA
            BitmapHelper.ConvertFromRGBAToBGRA(data.Scan0, data.Scan0, data.Width, data.Height);
            srcImg.UnlockBits(data);

            //Get the BarCodeFindContext
            var findContext = arr[0];

            //Generate the output image
            const int metadataHeight = 75;
            Bitmap dstImg = new Bitmap(srcImg.Width, srcImg.Height + metadataHeight, PixelFormat.Format32bppArgb);
            using (Graphics g = Graphics.FromImage(dstImg))
            {
                //Draw the background image
                g.DrawImage(srcImg, 0, 0, srcImg.Width, srcImg.Height);

                //Draw a bar for the metadata
                g.FillRectangle(Brushes.Black, 0, srcImg.Height, dstImg.Width, metadataHeight);
                
                float highestMatch = float.NaN;
                BarCodeAppearance highestAppearance = default(BarCodeAppearance);
                foreach(var appearance in findContext)
                {
                    g.DrawRectangle(Pens.Yellow, appearance.FirstBox);
                    g.DrawRectangle(Pens.Yellow, appearance.SecondBox);
                    g.DrawLine(Pens.Red, appearance.ColorStart, appearance.ColorEnd);
                    PointF scoreLabelPos = new PointF((appearance.ColorStart.X + appearance.ColorEnd.X) / 2, (appearance.ColorStart.Y + appearance.ColorEnd.Y) / 2);
                    g.DrawString(appearance.MatchScore.ToString("0.000f"), new Font(FontFamily.GenericSerif, 13), Brushes.Red, scoreLabelPos);

                    if (appearance.MatchScore > highestMatch || float.IsNaN(highestMatch))
                    {
                        highestMatch = appearance.MatchScore;
                        highestAppearance = appearance;
                    }
                }

                if(!float.IsNaN(highestMatch))
                {
                    g.DrawLine(Pens.Blue, highestAppearance.ColorStart, highestAppearance.ColorEnd);
                    g.DrawRectangle(Pens.Cyan, highestAppearance.FirstBox);
                    g.DrawRectangle(Pens.Cyan, highestAppearance.SecondBox);
                }

                string metadata = "[" + colorString + "] - " + (findContext.Count) + " appearances (highest match score: "+highestMatch.ToString("0.000")+")";
                g.DrawString(metadata, new Font(FontFamily.GenericSerif, 17), Brushes.White, new PointF(50, srcImg.Height));
            }

            //Save the labeled image
            string dstPath = path + ".labeled.png";
            dstImg.Save(dstPath);
            Console.WriteLine("Saved labeled image to "+dstPath);
        }
    }
}
