#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	//openFrameworksの初期設定
	ofSetWindowShape(1920, 1080);
	ofSetFrameRate(60);
	ofBackground(255, 255, 255);
	ofEnableAlphaBlending();

	//Kinectの接続確認
	if (!initKinect()) exit();

	/*
	//	手の描画処理の初期設定
	//	(i,j)は座標
	//	write[i][j] = true ならば(i,j)座標が描画される
	*/
	for (int i = 0; i < 1920; i++) {
		for (int j = 0; j < 1080; j++) {
			write[i][j] = false;
			writeLeft[i][j] = false;
			writeCount[i][j] = 0;
		}
	}

	_r = 0;
	_g = 0;
	_b = 0;

	//メモリ確保
	of_colorImage.allocate(colorWidth, colorHeight, OF_IMAGE_COLOR_ALPHA);
	colorImage.allocate(colorWidth, colorHeight);
	depthImage.allocate(depthWidth, depthHeight);
	depthImage_bin.allocate(depthWidth, depthHeight);
	gray.allocate(colorWidth, colorHeight);

	//閾値
	nearThreshold = 0;
	farThreshold = 256;

	//骨格のサークルのサイズ
	colorBodyCirclesSize = 50;

	//Box2Dの世界の設定
	box2d.init();	//初期化
	box2d.setGravity(0, 50);	//x,y方向の重力
	box2d.createBounds();	//画像の周辺に壁を作成
	box2d.drawGround();
	box2d.setFPS(60);	//FPS

						//頭と腕の骨格の線
	for (int i = 0; i < 10; i++) {
		skeltons[i] = ofPoint(0, 0);
	}

	frame_count = 0;
}

//--------------------------------------------------------------
void ofApp::update() {
	IColorFrame* colorFrame = nullptr;
	IDepthFrame* depthFrame = nullptr;
	IBodyFrame* bodyFrame = nullptr;

	HRESULT hResult = colorReader->AcquireLatestFrame(&colorFrame);
	HRESULT d_hResult = depthReader->AcquireLatestFrame(&depthFrame);

	//フレームの取得成功
	if (SUCCEEDED(hResult) && SUCCEEDED(d_hResult)) {

		//Kinectの設定______________________________________________________________
		hResult = colorFrame->CopyConvertedFrameDataToArray(colorHeight * colorWidth * colorBytesPerPixels, of_colorImage.getPixels(), ColorImageFormat_Rgba);
		d_hResult = depthFrame->CopyFrameDataToArray(depthBuffer.size(), &depthBuffer[0]);
		of_colorImage.update();	//ofImageのカラー画像のアップデート
		colorPixels = of_colorImage.getPixels();

		colorUpdate();	//ofxCvColorImage型 colorImage
		depthUpdate();	//ofxCvGrayscale型 depthImage;
		unsigned char* d_pixels = depthImage.getPixels();	//ビットマップ情報を配列に格納
		bodyUpdate();	//bodyのアップデート

		//circleが落ちたら上に戻ってくる_________________________________________________
		for (int i = 0; i < circles.size(); i++) {
			ofPoint point_tmp = circles.at(i).get()->getPosition();
			float s_x = colorWidth / 2 + ofRandom(-300, 300);
			if (point_tmp.y > ofGetHeight() - 50) {
				circles.at(i).get()->setPosition(s_x, 5);
			}
		}
		//_______________________________________________________________________

		//体に反応させる(5人まで処理可能)_____________________________________________
		for (int i = 0; i < colorBodyPoints.size(); i++) {
			if (colorBodyPoints[i].size() != 10)continue;
			for (int j = 0; j < colorBodyPoints[i].size() - 1; j++) {
				//頭⇒首は省略
				if (j == 5)continue;
				if (colorBodyPoints[i][j].x == 0 && colorBodyPoints[i][j].y == 0)continue;
				//インスタンス生成
				polyLines.push_back(ofPtr<ofxBox2dPolygon>(new ofxBox2dPolygon));
				//drawing
				ofPolyline drawing;
				drawing.addVertex(ofPoint(colorBodyPoints[i][j].x, colorBodyPoints[i][j].y + 10));
				drawing.addVertex(ofPoint(colorBodyPoints[i][j].x, colorBodyPoints[i][j].y - 10));
				drawing.addVertex(ofPoint(colorBodyPoints[i][j + 1].x, colorBodyPoints[i][j + 1].y - 10));
				drawing.addVertex(ofPoint(colorBodyPoints[i][j + 1].x, colorBodyPoints[i][j + 1].y + 10));
				drawing.setClosed(true);
				drawing.simplify();
				drawings.push_back(drawing);
				//polyLines
				polyLines.back().get()->addVertexes(drawing);
				polyLines.back().get()->simplify();
				polyLines.back().get()->setPhysics(0.0, 0.5, 0.5);
				polyLines.back().get()->create(box2d.getWorld());
			}
			//頭1
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircleクラスのインスタンス生成
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//（重さ、反発力、摩擦力）
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x, colorBodyPoints[i][5].y - 25, colorBodyCirclesSize);
			//頭2
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircleクラスのインスタンス生成
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//（重さ、反発力、摩擦力）
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x - 25, colorBodyPoints[i][5].y, colorBodyCirclesSize);
			//頭3
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircleクラスのインスタンス生成
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//（重さ、反発力、摩擦力）
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x + 25, colorBodyPoints[i][5].y, colorBodyCirclesSize);
			//頭4
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircleクラスのインスタンス生成
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//（重さ、反発力、摩擦力）
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x + 25, colorBodyPoints[i][5].y + 50, colorBodyCirclesSize);
			//頭5
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircleクラスのインスタンス生成
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//（重さ、反発力、摩擦力）
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x - 25, colorBodyPoints[i][5].y + 50, colorBodyCirclesSize);

		}

		//右手周辺領域を検出____________________________________________
		for (int i = 0; i < handPoints.size(); i++) {

			//Kinectが手の座標に抽出失敗していればcontinue
			if (handPoints[i].size() < 2)continue;

			//右手中心座標を取得
			ofPoint handCenter;
			handCenter.x = (handPoints[i][0].x + handPoints[i][1].x) / 2;
			handCenter.y = (handPoints[i][0].y + handPoints[i][1].y) / 2;

			//手領域に含まれる座標の深度値のうち、最初深度値を取得
			float thresh = 8000;	//thresh -> 最小深度値に更新される
			for (int y = -5; y < 5; y++) {
				for (int x = -5; x < 5; x++) {
					int _y = handCenter.y + y;
					int _x = handCenter.x + x;
					float tmp = depthBuffer[_y * depthImage.getWidth() + _x];
					if (thresh > tmp) {
						thresh = tmp;
					}
				}
			}

			//手領域の抽出処理・座標変換処理・手の塗りつぶし処理
			DepthSpacePoint d_p;
			vector<ofPoint> tmp_hand;
			for (int j = handPoints[i][0].y; j < handPoints[i][1].y; j++) {
				for (int k = handPoints[i][0].x; k < handPoints[i][1].x; k++) {
					//threshを用いて背景領域を削除し、手の領域のみを取得する
					int ptr = j * depthImage.getWidth() + k;
					float value = depthBuffer[ptr];
					if (value > thresh - 100 && value < thresh + 50) {
						//深度画像の背景を削除した手領域座標をカラー画像へ変換
						d_p.X = k;
						d_p.Y = j;
						ColorSpacePoint c_p;
						mapper->MapDepthPointToColorSpace(d_p, value, &c_p);
						//変換後の座標が画面外にはみ出していないか
						if ((int)c_p.X < 0 || (int)c_p.X >= colorWidth ||
							(int)c_p.Y < 0 || (int)c_p.Y >= colorHeight)continue;
						//変換した座標を保存
						tmp_hand.push_back(ofPoint((int)c_p.X, (int)c_p.Y));
						//手の周りを塗りつぶすために周り8近傍のwriteをtrueに
						for (int y = -2; y < 2; y++) {
							for (int x = -2; x < 2; x++) {
								int _x = (int)c_p.X + x;
								int _y = (int)c_p.Y + y;
								if (_x < 0 || _x >= colorWidth ||
									_y < 0 || _y >= colorHeight)continue;
								write[_x][_y] = true;
							}
						}
					}
				}
			}
			//ベクタに保存
			colorHandPoints.push_back(tmp_hand);

		}
		//________________________________________________________________


		//左手周辺領域を検出(右手と同じ処理)____________________________________________
		for (int i = 0; i < handPointsLeft.size(); i++) {
			if (handPointsLeft[i].size() < 2)continue;

			ofPoint handCenter;
			handCenter.x = (handPointsLeft[i][0].x + handPointsLeft[i][1].x) / 2;
			handCenter.y = (handPointsLeft[i][0].y + handPointsLeft[i][1].y) / 2;

			float thresh = 8000;
			for (int y = -5; y < 5; y++) {
				for (int x = -5; x < 5; x++) {
					int _y = handCenter.y + y;
					int _x = handCenter.x + x;
					float tmp = depthBuffer[_y * depthImage.getWidth() + _x];
					if (thresh > tmp) {
						thresh = tmp;
					}
				}
			}


			DepthSpacePoint d_p;
			vector<ofPoint> tmp_hand;
			for (int j = handPointsLeft[i][0].y; j < handPointsLeft[i][1].y; j++) {
				for (int k = handPointsLeft[i][0].x; k < handPointsLeft[i][1].x; k++) {

					int ptr = j * depthImage.getWidth() + k;
					float value = depthBuffer[ptr];
					if (value > thresh - 100 && value < thresh + 50) {

						d_p.X = k;
						d_p.Y = j;
						ColorSpacePoint c_p;
						mapper->MapDepthPointToColorSpace(d_p, value, &c_p);

						if ((int)c_p.X < 0 || (int)c_p.X >= colorWidth ||
							(int)c_p.Y < 0 || (int)c_p.Y >= colorHeight)continue;

						tmp_hand.push_back(ofPoint((int)c_p.X, (int)c_p.Y));

						for (int y = -2; y < 2; y++) {
							for (int x = -2; x < 2; x++) {
								int _x = (int)c_p.X + x;
								int _y = (int)c_p.Y + y;
								if (_x < 0 || _x >= colorWidth ||
									_y < 0 || _y >= colorHeight)continue;
								writeLeft[_x][_y] = true;
							}
						}
					}
				}
			}
			colorHandPoints.push_back(tmp_hand);

		}
		//________________________________________________________________

		//box2dのアップデート
		box2d.update();
	}

	//リリース
	SafeRelease(colorFrame);
	SafeRelease(depthFrame);
	SafeRelease(bodyFrame);
}

//--------------------------------------------------------------
void ofApp::draw() {

	//RGB画像の描画________________________________________________
	ofSetColor(255, 255, 255);
	colorImage.resize(1920, 1080);
	colorImage.draw(0, 0);
	//_____________________________________________________________

	ofSetBackgroundAuto(false);

	//落ちてくる円を描画_________________________________________
	for (int i = 0; i < circles.size(); i++) {
		ofSetColor(blue[i], green[i], red[i], 300);
		ofFill();
		circles[i].get()->draw();
	}
	//___________________________________________________________

	//骨格サークルの描画__________________________________________
	for (int i = 0; i < colorBodyCircles.size(); i++) {
		ofSetColor(0, 255, 0);
		ofFill();
		//colorBodyCircles[i].get()->draw();
	}
	//___________________________________________________________

	//完成した多角形の描画_______________________________________
	for (int i = 0; i < drawings.size(); i++) {
		ofSetColor(255, 0, 255);
		//drawings[i].draw();
	}
	//___________________________________________________________

	//リリース
	points.clear();
	contours.clear();
	cPoints.clear();
	cContours.clear();
	colorBodyPoints.clear();
	colorBodyCircles.clear();
	handPoints.clear();
	headPoints.clear();
	polyLines.clear();
	drawings.clear();
	colorMinDepthPoint.clear();
	colorHandPoints.clear();
	handPointsLeft.clear();
}

//Kinectの初期設定
bool ofApp::initKinect() {

	//Kinectを開く
	HRESULT hResult = S_OK;
	hResult = GetDefaultKinectSensor(&sensor);
	if (FAILED(hResult)) {
		std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
		return -1;
	}

	hResult = sensor->Open();
	if (FAILED(hResult)) {
		std::cerr << "Error : IKinectSensor::Open()" << std::endl;
		return -1;
	}

	//フレームソースの取得
	hResult = sensor->get_ColorFrameSource(&colorSource);
	if (FAILED(hResult)) {
		std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
		return -1;
	}
	hResult = sensor->get_DepthFrameSource(&depthSource);
	if (FAILED(hResult)) {
		std::cerr << "Error : IKinectSensor::get_DepthFrameSource()" << std::endl;
		return -1;
	}

	//リーダーを開く
	hResult = colorSource->OpenReader(&colorReader);
	if (FAILED(hResult)) {
		std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
		return -1;
	}
	hResult = depthSource->OpenReader(&depthReader);
	if (FAILED(hResult)) {
		std::cerr << "Error : IDepthFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	// ボディリーダーを取得する
	ComPtr<IBodyFrameSource> bodyFrameSource;
	ERROR_CHECK(sensor->get_BodyFrameSource(&bodyFrameSource));
	ERROR_CHECK(bodyFrameSource->OpenReader(&bodyReader));

	//フレームディスクリプションを作成
	hResult = colorSource->CreateFrameDescription(ColorImageFormat::ColorImageFormat_Rgba, &colorDescription);
	if (FAILED(hResult)) {
		std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
		return -1;
	}
	hResult = depthSource->get_FrameDescription(&depthDescription);
	if (FAILED(hResult)) {
		std::cerr << "Error : IDepthFrameSource::get_FrameDescription()" << std::endl;
		return -1;
	}

	colorDescription->get_Width(&colorWidth);
	colorDescription->get_Height(&colorHeight);
	colorDescription->get_BytesPerPixel(&colorBytesPerPixels);

	depthDescription->get_Width(&depthWidth);
	depthDescription->get_Height(&depthHeight);
	depthDescription->get_BytesPerPixel(&depthBytesPerPixels);

	//バッファーを作成
	depthBuffer.resize(depthWidth*depthHeight);

	//mapper
	hResult = sensor->get_CoordinateMapper(&mapper);
	if (FAILED(hResult)) {
		std::cerr << "Error : ComPtr<ICoordinateMapper::get_CoordinateMapper()" << std::endl;
		return -1;
	}

	return true;
}

void ofApp::colorUpdate() {

	//カラー画像のアップデート___________________________________________
	unsigned char* pixels = of_colorImage.getPixels();
	unsigned char* pix_cv = colorImage.getPixels();

	//手の描画処理
	for (int y = 0; y < colorHeight; y++) {
		for (int x = 0; x < colorWidth; x++) {
			int ptr = y * colorWidth * 4 + x * 4;
			int ptr_cv = y * colorWidth * 3 + x * 3;

			if (write[x][y]) {
				pix_cv[ptr_cv] = writeCount[x][y];
				pix_cv[ptr_cv + 1] = 255;
				pix_cv[ptr_cv + 2] = writeCount[x][y];


				writeCount[x][y]++;
				if (writeCount[x][y] >= MAX_WRITE_COUNT) {
					write[x][y] = false;
					writeCount[x][y] = 0;
				}
			}
			else if (writeLeft[x][y]) {
				pix_cv[ptr_cv] = 255;
				pix_cv[ptr_cv + 1] = writeCount[x][y];
				pix_cv[ptr_cv + 2] = 255;

				writeCount[x][y]++;
				if (writeCount[x][y] >= MAX_WRITE_COUNT) {
					writeLeft[x][y] = false;
					writeCount[x][y] = 0;
				}
			}
			else {
				pix_cv[ptr_cv] = pixels[ptr];
				pix_cv[ptr_cv + 1] = pixels[ptr + 1];
				pix_cv[ptr_cv + 2] = pixels[ptr + 2];
			}
		}
	}

	//colorImageの更新
	colorImage.setFromPixels(pix_cv, colorWidth, colorHeight);
	//___________________________________________________________
}

//nearThreshold, farThresholdの間にある深度領域のみ抽出する
void ofApp::depthUpdate()
{

	unsigned char* pixels = depthImage.getPixels();	//ビットマップ情報を配列に格納
	maxDepth = 0;
	minDepth = 255;
	for (int y = 0; y < depthImage.getHeight(); y++) {
		for (int x = 0; x < depthImage.getWidth(); x++) {
			int ptr = y * depthImage.getWidth() + x;
			int value = depthBuffer[ptr] * 255 / 8000;

			if (value >= nearThreshold && value <= farThreshold) {
				if (value > maxDepth)maxDepth = value;
				if (value < minDepth)minDepth = value;
				pixels[ptr] = value;
				points.push_back(ofPoint(x, y));
			}
			else {
				pixels[ptr] = 0;
			}
		}
	}
	depthImage.setFromPixels(pixels, depthImage.getWidth(), depthImage.getHeight());
}

void ofApp::bodyUpdate() {
	// フレームを取得する 
	ComPtr<IBodyFrame> bodyFrame;
	auto ret = bodyReader->AcquireLatestFrame(&bodyFrame);
	if (ret == S_OK) {
		// データを取得する
		ERROR_CHECK(bodyFrame->GetAndRefreshBodyData(6, &bodies[0]));
	}

	for (auto body : bodies) {
		if (body == nullptr) {
			continue;
		}

		BOOLEAN isTracked = false;
		ERROR_CHECK(body->get_IsTracked(&isTracked));
		if (!isTracked) {
			continue;
		}

		//関節の位置を表示する
		vector<ofPoint> bodyPoints(10);
		Joint joints[JointType::JointType_Count];
		body->GetJoints(JointType::JointType_Count, joints);
		for (auto joint : joints) {
			if (joint.TrackingState == TrackingState::TrackingState_Tracked) {

				//座標変換
				DepthSpacePoint point;
				ColorSpacePoint point2;
				CameraSpacePoint Pos = joint.Position;
				mapper->MapCameraPointToDepthSpace(Pos, &point);
				mapper->MapCameraPointToColorSpace(Pos, &point2);
				ofPoint d_p, c_p;
				d_p.x = (int)point.X;
				d_p.y = (int)point.Y;
				c_p.x = (int)point2.X;	//X閾値で判定
				c_p.y = (int)point2.Y;	//Y閾値で判定

				if (joint.JointType == JointType::JointType_HandRight) {
					vector<ofPoint> handRect;
					//右手
					bodyPoints[0] = c_p;
					//手の中心から左上
					ofPoint depthLeftUp;
					Pos.X -= range;
					Pos.Y += range;
					DepthSpacePoint tmp, tmp2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp);
					depthLeftUp.x = (int)tmp.X;
					depthLeftUp.y = (int)tmp.Y;
					handRect.push_back(depthLeftUp);
					//手の中心から右下
					ofPoint depthRightDown;
					Pos.X += range * 2;
					Pos.Y -= range * 2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp2);
					depthRightDown.x = (int)tmp2.X;
					depthRightDown.y = (int)tmp2.Y;
					handRect.push_back(depthRightDown);
					//保存
					handPoints.push_back(handRect);
				}
				else if (joint.JointType == JointType::JointType_ElbowRight) {
					//右ひじ
					bodyPoints[1] = c_p;
				}
				else if (joint.JointType == JointType::JointType_ShoulderRight) {
					//右かた
					bodyPoints[2] = c_p;
				}
				else if (joint.JointType == JointType::JointType_SpineShoulder) {
					//中央かた
					bodyPoints[3] = c_p;
				}
				else if (joint.JointType == JointType::JointType_Neck) {
					//首
					bodyPoints[4] = c_p;
					bodyPoints[6] = c_p;
				}
				else if (joint.JointType == JointType::JointType_Head) {
					//頭
					bodyPoints[5] = c_p;
				}
				else if (joint.JointType == JointType::JointType_ShoulderLeft) {
					//左かた
					bodyPoints[7] = c_p;
				}
				else if (joint.JointType == JointType::JointType_ElbowLeft) {
					//左ひじ
					bodyPoints[8] = c_p;
				}
				else if (joint.JointType == JointType::JointType_HandLeft) {
					//左手
					bodyPoints[9] = c_p;

					vector<ofPoint> handRect;
					//手の中心から左上
					ofPoint depthLeftUp;
					Pos.X -= range;
					Pos.Y += range;
					DepthSpacePoint tmp, tmp2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp);
					depthLeftUp.x = (int)tmp.X;
					depthLeftUp.y = (int)tmp.Y;
					handRect.push_back(depthLeftUp);
					//手の中心から右下
					ofPoint depthRightDown;
					Pos.X += range * 2;
					Pos.Y -= range * 2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp2);
					depthRightDown.x = (int)tmp2.X;
					depthRightDown.y = (int)tmp2.Y;
					handRect.push_back(depthRightDown);
					//保存
					handPointsLeft.push_back(handRect);

				}

			}
		}
		colorBodyPoints.push_back(bodyPoints);
		depthChange.push_back(false);
	}
}

void ofApp::setColorFromDepth(ofPoint p) {
	//カラー座標へ変換
	DepthSpacePoint dPoint;
	ColorSpacePoint cPoint;
	dPoint.X = (float)p.x;
	dPoint.Y = (float)p.y;
	mapper->MapDepthPointToColorSpace(dPoint, depthBuffer[p.y * depthImage.width + p.x], &cPoint);
	//範囲指定
	if ((int)cPoint.X < 0 || (int)cPoint.X >= colorWidth ||
		(int)cPoint.Y < 0 || (int)cPoint.Y >= colorHeight)return;
	//RGB値を取得
	int ptr_rgb = (int)cPoint.Y * colorWidth * 4 + (int)cPoint.X * 4;
	int b = colorPixels[ptr_rgb];
	int g = colorPixels[ptr_rgb + 1];
	int r = colorPixels[ptr_rgb + 2];
	ofSetColor(b, g, r);
}

ofPoint ofApp::getColorFromDepth(ofPoint p) {
	//カラー座標へ変換
	DepthSpacePoint dPoint;
	ColorSpacePoint cPoint;
	dPoint.X = (float)p.x;
	dPoint.Y = (float)p.y;
	mapper->MapDepthPointToColorSpace(dPoint, depthBuffer[p.y * depthImage.width + p.x], &cPoint);
	//範囲指定
	if ((int)cPoint.X < 0 || (int)cPoint.X >= colorWidth ||
		(int)cPoint.Y < 0 || (int)cPoint.Y >= colorHeight) {
		return ofPoint(0, 0);
	}
	else {
		return ofPoint((int)cPoint.X, (int)cPoint.Y);
	}
}

//--------------------------------------------------------------

//fが入力されるとcircleを発生
void ofApp::keyPressed(int key) {
	if (key == 'f') {
		//circleの作成_______________________________________________________________________
		if (circles.size() < 500) {
			//if (frame_count % 2 == 0)continue;
			float ram_size = ofRandom(5, 15);
			float ram_location = ofRandom(100, 1700);
			float ram_weight = ofRandom(10, 50);
			float s_x = colorWidth / 2 + ofRandom(-300, 300);

			circles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircleクラスのインスタンス生成
			circles.back().get()->setPhysics(100, 0.53, 0.1);	//（重さ、反発力、摩擦力）
			circles.back().get()->setup(box2d.getWorld(), s_x, 5, ram_size);//x,y,r
			circles.back().get()->addForce(ofVec2f(force_x, force_y), force);
			red.push_back(ofRandom(0, 255));
			green.push_back(ofRandom(0, 255));
			blue.push_back(ofRandom(0, 255));
		}
		//__________________________________________________________________________________
	}

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

ofPoint ofApp::setColorRGB() {
	int ram = ofRandom(0, 2);
	int move = 0;
	if (ram == 0) {
		//赤
		if (_r >= 255) {
			move = -move_value;
		}
		else {
			move = +move_value;
		}
	}
	else if (ram == 1) {
		//緑
		if (_g >= 255) {
			move = -move_value;
		}
		else {
			move = move_value;
		}
	}
	else {
		//青
		if (_b >= 255) {
			move = -move_value;
		}
		else {
			move = move_value;
		}
	}

	return ofPoint(ram, move);


}