// OpenCVTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <direct.h>
#include <Windows.h>
#include <string>
#include <fstream>
#include <algorithm>
#include <tesseract\baseapi.h>
#include <time.h>
using namespace cv;
using namespace std;
using namespace tesseract;

Scalar getMSSIM(const Mat& i1, const Mat& i2)
{
	const double C1 = 6.5025, C2 = 58.5225;
	/***************************** INITS **********************************/
	int d = CV_32F;

	Mat I1, I2;
	i1.convertTo(I1, d);            // cannot calculate on one byte large values
	i2.convertTo(I2, d);

	Mat I2_2 = I2.mul(I2);        // I2^2
	Mat I1_2 = I1.mul(I1);        // I1^2
	Mat I1_I2 = I1.mul(I2);        // I1 * I2

	/*************************** END INITS **********************************/

	Mat mu1, mu2;                   // PRELIMINARY COMPUTING
	GaussianBlur(I1, mu1, Size(11, 11), 1.5);
	GaussianBlur(I2, mu2, Size(11, 11), 1.5);

	Mat mu1_2 = mu1.mul(mu1);
	Mat mu2_2 = mu2.mul(mu2);
	Mat mu1_mu2 = mu1.mul(mu2);

	Mat sigma1_2, sigma2_2, sigma12;

	GaussianBlur(I1_2, sigma1_2, Size(11, 11), 1.5);
	sigma1_2 -= mu1_2;

	GaussianBlur(I2_2, sigma2_2, Size(11, 11), 1.5);
	sigma2_2 -= mu2_2;

	GaussianBlur(I1_I2, sigma12, Size(11, 11), 1.5);
	sigma12 -= mu1_mu2;

	///////////////////////////////// FORMULA ////////////////////////////////
	Mat t1, t2, t3;

	t1 = 2 * mu1_mu2 + C1;
	t2 = 2 * sigma12 + C2;
	t3 = t1.mul(t2);                 // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

	t1 = mu1_2 + mu2_2 + C1;
	t2 = sigma1_2 + sigma2_2 + C2;
	t1 = t1.mul(t2);                 // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

	Mat ssim_map;
	divide(t3, t1, ssim_map);        // ssim_map =  t3./t1;

	Scalar mssim = mean(ssim_map);   // mssim = average of ssim map
	return mssim;
}
double getPSNR(const Mat& I1, const Mat& I2)
{
	Mat s1;
	absdiff(I1, I2, s1);       // |I1 - I2|
	
	s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
	s1 = s1.mul(s1);           // |I1 - I2|^2

	Scalar s = sum(s1);        // sum elements per channel

	double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

	if (sse <= 1e-10) // for small values return zero
		return 0;
	else
	{
		double mse = sse / (double)(I1.channels() * I1.total());
		//cout << (I1.channels()) << " " << I1.total() << endl;
		double psnr = 10.0 * log10((255 * 255) / mse);
		return psnr;
	}
}
double getPSNR2(const Mat& I1, const Mat& I2)
{
	Mat s1;
	absdiff(I1, I2, s1);       // |I1 - I2|
	namedWindow("Thumbs", WINDOW_AUTOSIZE);
	imshow("Thumbs", I2);
	waitKey(0);
	s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
	s1 = s1.mul(s1);           // |I1 - I2|^2

	Scalar s = sum(s1);        // sum elements per channel

	double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

	if (sse <= 1e-10) // for small values return zero
		return 0;
	else
	{
		double mse = sse / (double)(I1.channels() * I1.total());
		//cout << (I1.channels()) << " " << I1.total() << endl;
		double psnr = 10.0 * log10((255 * 255) / mse);
		return psnr;
	}
}

string convertInt(int number)
{
	stringstream ss;//create a stringstream
	ss << number;//add number to the stream
	return ss.str();//return a string with the contents of the stream
}

int SearchDirectory(std::vector<std::string> &refvecFiles,
	const std::string		&refcstrRootDirectory,
	const std::string		&refcstrExtension,
	bool					 bSearchSubdirectories = true)
{
	std::string	 strFilePath;			 // Filepath
	std::string	 strPattern;			  // Pattern
	std::string	 strExtension;			// Extension
	HANDLE		  hFile;				   // Handle to file
	WIN32_FIND_DATAA FileInformation;		 // File information


	strPattern = refcstrRootDirectory + "\\*.*";

	hFile = ::FindFirstFileA(strPattern.c_str(), &FileInformation);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FileInformation.cFileName[0] != '.')
			{
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (bSearchSubdirectories)
					{
						// Search subdirectory
						int iRC = SearchDirectory(refvecFiles,
							strFilePath,
							refcstrExtension,
							bSearchSubdirectories);
						if (iRC)
							return iRC;
					}
				}
				else
				{
					// Check extension
					strExtension = FileInformation.cFileName;
					strExtension = strExtension.substr(strExtension.rfind(".") + 1);

					if (strExtension == refcstrExtension)
					{
						// Save filename
						refvecFiles.push_back(strFilePath);
					}
				}
			}
		} while (::FindNextFileA(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
			return dwError;
	}

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	//Load Champion References
	vector<string> imgFiles;
	int iRC = 0;
	iRC = SearchDirectory(imgFiles, "squares", "png");
	if (iRC){
		return -1;
	}
	map < string, Mat > championMatMap;
	for (auto imgName : imgFiles){
		championMatMap[imgName] = imread(imgName);

	}

	//Load Summoner Spell References
	vector<string> summonerName;
	ifstream summonerNameIn("resources\\ss.txt");
	if (summonerNameIn.is_open()){
		for (int i = 0; i < 12; i++){
			string temp;
			getline(summonerNameIn, temp);
			summonerName.push_back(temp);
		}
	}
	summonerNameIn.close();

	map< string, Mat> summonersMatMap;
	Mat rawSummoners = imread("resources\\ss.png");
	for (int i = 0; i < 12; i++){
		Rect temp(i * 48, 0, 48, 48);
		summonersMatMap[summonerName[i]] = rawSummoners(temp);
	}

	//Load Item References
	vector<string> itemName;
	ifstream itemNameIn("resources\\items.txt");
	if (itemNameIn.is_open()){
		while(!itemNameIn.eof()){
			string temp;
			getline(itemNameIn, temp);
			itemName.push_back(temp);
		}
	}
	itemNameIn.close();
	//namedWindow("Thumbs", WINDOW_AUTOSIZE);
	map< string, Mat> itemMatMap;
	Mat rawItems = imread("resources\\items.png");
	for (int i = 0; i < 10; i++){
		for (int j = 0; j < 19; j++){
			Rect temp(j * 48, i * 48, 48, 48);
			cout << i << " av " << j << endl;
			itemMatMap[itemName[j + i * 19]] = rawItems(temp);
			//imshow("Thumbs", rawItems(temp));
			//waitKey(0);
		}
	}

	for (map<string, Mat>::const_iterator it = itemMatMap.begin(); it != itemMatMap.end(); ++it){
		cout << it->first << endl;	
	}
	int ss = 10;
	Mat screen = imread("resources\\screen"+convertInt(ss)+".png");

	//Read Champions from Screenshot
	vector<Mat> championThumbs;
	for (int i = 0; i < 5; i++){
		Rect temp(26, 154 + i * 70, 32, 32);
		championThumbs.push_back(screen(temp));
	}
	for (int i = 0; i < 5; i++){
		Rect temp(1222, 154 + i * 70, 32, 32);
		championThumbs.push_back(screen(temp));
	}

	//Read Summoners from Screenshot
	vector<Mat> summonerThumbs;
	for (int i = 0; i < 5; i++){
		for (int j = 0; j < 2; j++){
			Rect temp(5, 171 + i * 70 + j * 18, 16, 16);
			summonerThumbs.push_back(screen(temp));
		}
	}
	for (int i = 0; i < 5; i++){
		for (int j = 0; j < 2; j++){
			Rect temp(1258, 171 + i * 70 + j * 18, 16, 16);
			summonerThumbs.push_back(screen(temp));
		}
	}
	//for (auto img : summonerThumbs){
	//	namedWindow("Thumbs", WINDOW_AUTOSIZE);
	//	imshow("Thumbs", img);
	//	waitKey(0);
	//}

	//Read Items from Screenshot
	vector<vector<Mat>> itemThumbs;
	for (int i = 0; i < 5; i++){
		vector<Mat> currentChampion;
		for (int j = 0; j < 6; j++){
			Rect temp(417 + j * 17, min(622 + i * 20,720-16), 16, 16);
			cout << 417 + j * 18 << " " << min(622 + i * 20, 720 - 16) << endl;
			
			currentChampion.push_back(screen(temp));
		}
		itemThumbs.push_back(currentChampion);
	}
	for (int i = 0; i < 5; i++){
		vector<Mat> currentChampion;
		for (int j = 0; j < 6; j++){
			cout << 762 + j * 17 << " " << min(622 + i * 20, 720 - 16) << endl;
			Rect temp(762 + j * 18, min(622 + i * 20, 720 - 16), 16, 16);
			
			currentChampion.push_back(screen(temp));
		}
		itemThumbs.push_back(currentChampion);
	}
	for (auto i : itemThumbs){
		for (auto j : i){
			imshow("Thumbs", j);
			waitKey(0);
		}
	}

	//Detect Champions
	for (auto img : championThumbs){
		double bestPSNR = 0;
		string bestMatch;
		for (auto imgName : imgFiles){
			Mat src2 = championMatMap[imgName];
			Mat src2small;
			resize(src2, src2small, img.size());
			double psnr = getPSNR(img, src2small);
			if (psnr >= bestPSNR){
				bestPSNR = psnr;
				bestMatch = imgName;
			}
		}
		cout << "Best Match: " << bestMatch << " Score: " << bestPSNR << endl;
		//namedWindow("Thumbs", WINDOW_AUTOSIZE);
		//imshow("Thumbs", img);
		//waitKey(0);
	}
	//Detect Summoners
	for (auto img : summonerThumbs){
		double bestPSNR = 0;
		string bestMatch;
		for (map<string, Mat>::const_iterator it = summonersMatMap.begin(); it != summonersMatMap.end(); ++it){
			Mat src2 = summonersMatMap[it->first];
			Mat src2small;
			resize(src2, src2small, img.size());
			double psnr = getPSNR(img, src2small);
			if (psnr >= bestPSNR){
				bestPSNR = psnr;
				bestMatch = it->first;
			}
		}
		cout << "Best Match: " << bestMatch << " Score: " << bestPSNR << endl;
		//namedWindow("Thumbs", WINDOW_AUTOSIZE);
		//imshow("Thumbs", img);
		//waitKey(0);
	}
	//Detect items
	for (auto champion : itemThumbs){
		for (auto item : champion){
			double bestPSNR = 0;
			string bestMatch;
			for (map<string, Mat>::const_iterator it = itemMatMap.begin(); it != itemMatMap.end(); ++it){
				Mat src2 = itemMatMap[it->first];
				Mat src2small;
				//cout << it->first << endl;
				resize(src2, src2small, item.size());
				double psnr = getPSNR(item, src2small);
				if (psnr >= bestPSNR){
					bestPSNR = psnr;
					bestMatch = it->first;
				}
			}
			cout << "Best Match: " << bestMatch << " Score: " << bestPSNR << endl;
			//namedWindow("Thumbs1", WINDOW_AUTOSIZE);
			//namedWindow("Thumbs2", WINDOW_AUTOSIZE);
			//imshow("Thumbs1", itemMatMap[bestMatch]);
			//imshow("Thumbs2", item);
			//waitKey(0);
		}
	}
	//OCR Test
	Mat OCRText = screen(Rect(528, 620, 91, 100));
	Mat OCRText2 = screen(Rect(662, 620, 91, 100));
	Mat screenGray;
	Mat screenGray2;
	cvtColor(OCRText, screenGray, CV_RGB2GRAY);
	cvtColor(OCRText2, screenGray2, CV_RGB2GRAY);
	Mat invert = cv::Scalar::all(255) - screenGray;
	Mat invert2 = cv::Scalar::all(255) - screenGray2;
	Mat thresholdtest;
	Mat thresholdtest2;
	adaptiveThreshold(invert, thresholdtest, 255.0, CV_THRESH_BINARY, CV_ADAPTIVE_THRESH_MEAN_C, 11, 11);
	adaptiveThreshold(invert2, thresholdtest2, 255.0, CV_THRESH_BINARY, CV_ADAPTIVE_THRESH_MEAN_C, 11, 11);

	//////////////////
	time_t timer;
	struct tm y2k;
	double seconds;

	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time(&timer);

	seconds = difftime(timer, mktime(&y2k));
	string t = convertInt((int)seconds);

	//imwrite("training\\lol.f.exp" + convertInt(ss * 2 - 1) + ".tif", thresholdtest);
	//imwrite("training\\lol.f.exp" + convertInt(ss * 2) + ".tif", thresholdtest2);
	////////////////////
	TessBaseAPI tess;
	tess.Init("", "lol", tesseract::OEM_DEFAULT);
	tess.SetVariable("tessedit_char_whitelist", "1234567890/");
	cout << "av" << endl;
	tess.SetImage((uchar*)thresholdtest.data, thresholdtest.size().width, thresholdtest.size().height, thresholdtest.channels(), thresholdtest.step1());
	tess.Recognize(0);
	const char* out = tess.GetUTF8Text();
	cout << out << endl;
	tess.SetImage((uchar*)thresholdtest2.data, thresholdtest2.size().width, thresholdtest2.size().height, thresholdtest2.channels(), thresholdtest2.step1());
	tess.Recognize(0);
	const char* out2 = tess.GetUTF8Text();
	cout << out2 << endl;
	int av;
	cin >> av;
	return 0;
}

