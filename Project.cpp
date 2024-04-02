#include<opencv2/imgcodecs.hpp>
#include<opencv2/highgui.hpp>
#include<opencv2/imgproc.hpp>
#include<iostream>
#include<vector>

using namespace std;
using namespace cv;
/*Idea behind the peoject is that we will first convert the image to gray scale
then we will use blur (preproccessing) and then canny edge detector to detect all the edges
(of biggest reactangle in our images assuming it is the paper)
,then after getting edges we will get the idea where our paper is and then we can get co-
ordinates and then we will extract all 4 corners of our paper and we will then warp our image
to get a scanned view*/


Mat img, imgGray, imgCanny, imgThreshold, imgBlur, imgDil, imgErode, imgWarp, imgCrop;
vector<Point> initialPoints, docPoints;
float w = 420, h = 596; //width and height of a-4 paper are quite small so unko 2 se multiply are diya

Mat preProcessing(Mat img)
{
	cvtColor(img, imgGray, COLOR_BGR2GRAY);
	GaussianBlur(imgGray, imgBlur, Size(3, 3), 3, 0);
	Canny(imgBlur, imgCanny, 25, 75);

	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	dilate(imgCanny, imgDil, kernel);
	//erode(imgDil, imgErode, kernel);
	return imgDil;
}

vector <Point> getContours(Mat image) {
	vector<vector<Point>> contours;
	vector<Vec4i> heirarchy;


	findContours(imgDil, contours, heirarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	vector<vector<Point>> conPoly(contours.size()); //har ek shape k bounderies k vector ak vector
	vector<Rect> boundRect(contours.size());

	vector <Point> biggest; //biggest rectangle shape in the picture k co-ordinates ka array
	//we will return this
	int maxArea = 0;

	for (int i = 0; i < contours.size(); i++) //saare detected contours pe iterate karega
	{
		int area = contourArea(contours[i]);
		cout << area << endl;
		string objectType;
		if (area > 1000)
		{
			float peri = arcLength(contours[i], true);
			approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);
			if (area > maxArea && conPoly[i].size() == 4)
				/*area max area se bada hua toh ab naya max area mil jayega and ye condition
				tabhi jab karni h jab object or contour ek rectangle ho ie uski 4 outlines hoan*/

			{
				//drawContours(img, conPoly, i, Scalar(255, 0, 255), 5); //ye pehle experiment stage 
				// mein bavaya tha, end result mein nahi chahiye
				//sirf bade rectnagles draw karenge
				biggest = { conPoly[i][0],conPoly[i][1] ,conPoly[i][2] ,conPoly[i][3] };
				//stored all 4 co-ordinates
				maxArea = area;
			}


			//rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0), 5);
		}
	}
	return biggest;

	/*iss function mein cotours(outlines) detect hongi and largest peri waale shap k co - ordinates
	return kar denge */

}

void drawPoints(vector<Point> points, Scalar color)
{
	for (int i = 0; i < points.size(); i++) {
		circle(img, points[i], 10, color, FILLED);
		putText(img, to_string(i), points[i], FONT_HERSHEY_PLAIN, 4, color, 4);
	}
	/*jitne points bheje hain ie rectangle k co - ordinates or sabse bade contour k co - ordinates
	jo bheje hain unpe har ek pe circle draw karenge aur text likhenge ki vo kitne no ka pt h */
}

/*Why are we reordering points?-
Because everytime the algorithm detects a new document or rectangle it will give co-ordinates
in different order ie points 0,1,2,3 or to different co-ordinates toh so that ki points ki
ordering sahi ho we are doing this, ordering will be like top left corner will be pt 0,
top right-1 bottom left-2 botoom right-1 and the way to give them numbering is-
x and y are x and y co-ordinates of each point-
0-x+y least
1-x-y max
2-x-y least
3-x+y max
*/

vector<Point> reorderPoints(vector<Point> points)
{
	vector<Point> newPoints;
	vector<int> sumPoints, subPoints;

	for (int i = 0; i < 4; i++) {
		sumPoints.push_back(points[i].x + points[i].y);
		subPoints.push_back(points[i].x - points[i].y);
	}

	newPoints.push_back(points[min_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]); //0
	newPoints.push_back(points[max_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]); //1
	newPoints.push_back(points[min_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]); //2
	newPoints.push_back(points[max_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]); //3

	return newPoints;
}

Mat getWarp(Mat img, vector<Point> points, float w, float h)
{
	Point2f src[4] = { points[0],points[1],points[2],points[3] };
	Point2f dst[4] = { {0.0f,0.0f},{w,0.0f},{0.0f,h},{w,h} };

	Mat matrix = getPerspectiveTransform(src, dst);
	warpPerspective(img, imgWarp, matrix, Point(w, h));

	return imgWarp;
}

void main() {

	string path = "Resources/paper.jpg";
	img = imread(path);
	//resize(img, img, Size(), 0.5, 0.5); 
	/*resized image to half size for clear view
	zyada badi h initially (Will remove later after final editing and cropping is done)
		so that vo original image se le document and clear document aaye */

		//Preprocessing
	imgThreshold = preProcessing(img);

	//getContours(outlines) of biggest possible rect
	initialPoints = getContours(imgThreshold);
	//inko aur process karenge abhi
	docPoints = reorderPoints(initialPoints);
	//drawPoints(docPoints, Scalar(0, 255, 0));
	imshow("Image", img);
	//imshow("Dilated Image", imgThreshold);

	//Warp Image 

	imgWarp = getWarp(img, docPoints, w, h); //w,h will be 2 times the size of a-4 sheet for clear view
	//imshow("Scanned Document", imgWarp);
	//lekin abhi thodi image distorted lag rahi h perfect document nahi dikh raha toh we need to crop
	//edges buri lag rahi hain, document natural nahi lag raha isliye cropping
	int cropVal = 10;
	Rect roi(cropVal, cropVal, w - (2 * cropVal), h - (2 * cropVal)); /*harside se 10 pixels hata
	diye ie width, height se 5 px each */
	/*w - (2 * cropVal) and same for height isliye kiya as dono side se 5 px hata diye toh total
	width se 2*cropVal hat gaye */
	imgCrop = imgWarp(roi);
	imshow("Scanned Document", imgCrop);
	waitKey(0);
}

