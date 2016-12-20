#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	//openFrameworks�̏����ݒ�
	ofSetWindowShape(1920, 1080);
	ofSetFrameRate(60);
	ofBackground(255, 255, 255);
	ofEnableAlphaBlending();

	//Kinect�̐ڑ��m�F
	if (!initKinect()) exit();

	/*
	//	��̕`�揈���̏����ݒ�
	//	(i,j)�͍��W
	//	write[i][j] = true �Ȃ��(i,j)���W���`�悳���
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

	//�������m��
	of_colorImage.allocate(colorWidth, colorHeight, OF_IMAGE_COLOR_ALPHA);
	colorImage.allocate(colorWidth, colorHeight);
	depthImage.allocate(depthWidth, depthHeight);
	depthImage_bin.allocate(depthWidth, depthHeight);
	gray.allocate(colorWidth, colorHeight);

	//臒l
	nearThreshold = 0;
	farThreshold = 256;

	//���i�̃T�[�N���̃T�C�Y
	colorBodyCirclesSize = 50;

	//Box2D�̐��E�̐ݒ�
	box2d.init();	//������
	box2d.setGravity(0, 50);	//x,y�����̏d��
	box2d.createBounds();	//�摜�̎��ӂɕǂ��쐬
	box2d.drawGround();
	box2d.setFPS(60);	//FPS

						//���Ƙr�̍��i�̐�
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

	//�t���[���̎擾����
	if (SUCCEEDED(hResult) && SUCCEEDED(d_hResult)) {

		//Kinect�̐ݒ�______________________________________________________________
		hResult = colorFrame->CopyConvertedFrameDataToArray(colorHeight * colorWidth * colorBytesPerPixels, of_colorImage.getPixels(), ColorImageFormat_Rgba);
		d_hResult = depthFrame->CopyFrameDataToArray(depthBuffer.size(), &depthBuffer[0]);
		of_colorImage.update();	//ofImage�̃J���[�摜�̃A�b�v�f�[�g
		colorPixels = of_colorImage.getPixels();

		colorUpdate();	//ofxCvColorImage�^ colorImage
		depthUpdate();	//ofxCvGrayscale�^ depthImage;
		unsigned char* d_pixels = depthImage.getPixels();	//�r�b�g�}�b�v����z��Ɋi�[
		bodyUpdate();	//body�̃A�b�v�f�[�g

		//circle�����������ɖ߂��Ă���_________________________________________________
		for (int i = 0; i < circles.size(); i++) {
			ofPoint point_tmp = circles.at(i).get()->getPosition();
			float s_x = colorWidth / 2 + ofRandom(-300, 300);
			if (point_tmp.y > ofGetHeight() - 50) {
				circles.at(i).get()->setPosition(s_x, 5);
			}
		}
		//_______________________________________________________________________

		//�̂ɔ���������(5�l�܂ŏ����\)_____________________________________________
		for (int i = 0; i < colorBodyPoints.size(); i++) {
			if (colorBodyPoints[i].size() != 10)continue;
			for (int j = 0; j < colorBodyPoints[i].size() - 1; j++) {
				//���ˎ�͏ȗ�
				if (j == 5)continue;
				if (colorBodyPoints[i][j].x == 0 && colorBodyPoints[i][j].y == 0)continue;
				//�C���X�^���X����
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
			//��1
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircle�N���X�̃C���X�^���X����
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//�i�d���A�����́A���C�́j
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x, colorBodyPoints[i][5].y - 25, colorBodyCirclesSize);
			//��2
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircle�N���X�̃C���X�^���X����
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//�i�d���A�����́A���C�́j
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x - 25, colorBodyPoints[i][5].y, colorBodyCirclesSize);
			//��3
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircle�N���X�̃C���X�^���X����
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//�i�d���A�����́A���C�́j
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x + 25, colorBodyPoints[i][5].y, colorBodyCirclesSize);
			//��4
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircle�N���X�̃C���X�^���X����
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//�i�d���A�����́A���C�́j
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x + 25, colorBodyPoints[i][5].y + 50, colorBodyCirclesSize);
			//��5
			colorBodyCircles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircle�N���X�̃C���X�^���X����
			colorBodyCircles.back().get()->setPhysics(1, 0.3, 0.1);	//�i�d���A�����́A���C�́j
			colorBodyCircles.back().get()->setup(box2d.getWorld(), colorBodyPoints[i][5].x - 25, colorBodyPoints[i][5].y + 50, colorBodyCirclesSize);

		}

		//�E����ӗ̈�����o____________________________________________
		for (int i = 0; i < handPoints.size(); i++) {

			//Kinect����̍��W�ɒ��o���s���Ă����continue
			if (handPoints[i].size() < 2)continue;

			//�E�蒆�S���W���擾
			ofPoint handCenter;
			handCenter.x = (handPoints[i][0].x + handPoints[i][1].x) / 2;
			handCenter.y = (handPoints[i][0].y + handPoints[i][1].y) / 2;

			//��̈�Ɋ܂܂����W�̐[�x�l�̂����A�ŏ��[�x�l���擾
			float thresh = 8000;	//thresh -> �ŏ��[�x�l�ɍX�V�����
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

			//��̈�̒��o�����E���W�ϊ������E��̓h��Ԃ�����
			DepthSpacePoint d_p;
			vector<ofPoint> tmp_hand;
			for (int j = handPoints[i][0].y; j < handPoints[i][1].y; j++) {
				for (int k = handPoints[i][0].x; k < handPoints[i][1].x; k++) {
					//thresh��p���Ĕw�i�̈���폜���A��̗̈�݂̂��擾����
					int ptr = j * depthImage.getWidth() + k;
					float value = depthBuffer[ptr];
					if (value > thresh - 100 && value < thresh + 50) {
						//�[�x�摜�̔w�i���폜������̈���W���J���[�摜�֕ϊ�
						d_p.X = k;
						d_p.Y = j;
						ColorSpacePoint c_p;
						mapper->MapDepthPointToColorSpace(d_p, value, &c_p);
						//�ϊ���̍��W����ʊO�ɂ͂ݏo���Ă��Ȃ���
						if ((int)c_p.X < 0 || (int)c_p.X >= colorWidth ||
							(int)c_p.Y < 0 || (int)c_p.Y >= colorHeight)continue;
						//�ϊ��������W��ۑ�
						tmp_hand.push_back(ofPoint((int)c_p.X, (int)c_p.Y));
						//��̎����h��Ԃ����߂Ɏ���8�ߖT��write��true��
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
			//�x�N�^�ɕۑ�
			colorHandPoints.push_back(tmp_hand);

		}
		//________________________________________________________________


		//������ӗ̈�����o(�E��Ɠ�������)____________________________________________
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

		//box2d�̃A�b�v�f�[�g
		box2d.update();
	}

	//�����[�X
	SafeRelease(colorFrame);
	SafeRelease(depthFrame);
	SafeRelease(bodyFrame);
}

//--------------------------------------------------------------
void ofApp::draw() {

	//RGB�摜�̕`��________________________________________________
	ofSetColor(255, 255, 255);
	colorImage.resize(1920, 1080);
	colorImage.draw(0, 0);
	//_____________________________________________________________

	ofSetBackgroundAuto(false);

	//�����Ă���~��`��_________________________________________
	for (int i = 0; i < circles.size(); i++) {
		ofSetColor(blue[i], green[i], red[i], 300);
		ofFill();
		circles[i].get()->draw();
	}
	//___________________________________________________________

	//���i�T�[�N���̕`��__________________________________________
	for (int i = 0; i < colorBodyCircles.size(); i++) {
		ofSetColor(0, 255, 0);
		ofFill();
		//colorBodyCircles[i].get()->draw();
	}
	//___________________________________________________________

	//�����������p�`�̕`��_______________________________________
	for (int i = 0; i < drawings.size(); i++) {
		ofSetColor(255, 0, 255);
		//drawings[i].draw();
	}
	//___________________________________________________________

	//�����[�X
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

//Kinect�̏����ݒ�
bool ofApp::initKinect() {

	//Kinect���J��
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

	//�t���[���\�[�X�̎擾
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

	//���[�_�[���J��
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

	// �{�f�B���[�_�[���擾����
	ComPtr<IBodyFrameSource> bodyFrameSource;
	ERROR_CHECK(sensor->get_BodyFrameSource(&bodyFrameSource));
	ERROR_CHECK(bodyFrameSource->OpenReader(&bodyReader));

	//�t���[���f�B�X�N���v�V�������쐬
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

	//�o�b�t�@�[���쐬
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

	//�J���[�摜�̃A�b�v�f�[�g___________________________________________
	unsigned char* pixels = of_colorImage.getPixels();
	unsigned char* pix_cv = colorImage.getPixels();

	//��̕`�揈��
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

	//colorImage�̍X�V
	colorImage.setFromPixels(pix_cv, colorWidth, colorHeight);
	//___________________________________________________________
}

//nearThreshold, farThreshold�̊Ԃɂ���[�x�̈�̂ݒ��o����
void ofApp::depthUpdate()
{

	unsigned char* pixels = depthImage.getPixels();	//�r�b�g�}�b�v����z��Ɋi�[
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
	// �t���[�����擾���� 
	ComPtr<IBodyFrame> bodyFrame;
	auto ret = bodyReader->AcquireLatestFrame(&bodyFrame);
	if (ret == S_OK) {
		// �f�[�^���擾����
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

		//�֐߂̈ʒu��\������
		vector<ofPoint> bodyPoints(10);
		Joint joints[JointType::JointType_Count];
		body->GetJoints(JointType::JointType_Count, joints);
		for (auto joint : joints) {
			if (joint.TrackingState == TrackingState::TrackingState_Tracked) {

				//���W�ϊ�
				DepthSpacePoint point;
				ColorSpacePoint point2;
				CameraSpacePoint Pos = joint.Position;
				mapper->MapCameraPointToDepthSpace(Pos, &point);
				mapper->MapCameraPointToColorSpace(Pos, &point2);
				ofPoint d_p, c_p;
				d_p.x = (int)point.X;
				d_p.y = (int)point.Y;
				c_p.x = (int)point2.X;	//X臒l�Ŕ���
				c_p.y = (int)point2.Y;	//Y臒l�Ŕ���

				if (joint.JointType == JointType::JointType_HandRight) {
					vector<ofPoint> handRect;
					//�E��
					bodyPoints[0] = c_p;
					//��̒��S���獶��
					ofPoint depthLeftUp;
					Pos.X -= range;
					Pos.Y += range;
					DepthSpacePoint tmp, tmp2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp);
					depthLeftUp.x = (int)tmp.X;
					depthLeftUp.y = (int)tmp.Y;
					handRect.push_back(depthLeftUp);
					//��̒��S����E��
					ofPoint depthRightDown;
					Pos.X += range * 2;
					Pos.Y -= range * 2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp2);
					depthRightDown.x = (int)tmp2.X;
					depthRightDown.y = (int)tmp2.Y;
					handRect.push_back(depthRightDown);
					//�ۑ�
					handPoints.push_back(handRect);
				}
				else if (joint.JointType == JointType::JointType_ElbowRight) {
					//�E�Ђ�
					bodyPoints[1] = c_p;
				}
				else if (joint.JointType == JointType::JointType_ShoulderRight) {
					//�E����
					bodyPoints[2] = c_p;
				}
				else if (joint.JointType == JointType::JointType_SpineShoulder) {
					//��������
					bodyPoints[3] = c_p;
				}
				else if (joint.JointType == JointType::JointType_Neck) {
					//��
					bodyPoints[4] = c_p;
					bodyPoints[6] = c_p;
				}
				else if (joint.JointType == JointType::JointType_Head) {
					//��
					bodyPoints[5] = c_p;
				}
				else if (joint.JointType == JointType::JointType_ShoulderLeft) {
					//������
					bodyPoints[7] = c_p;
				}
				else if (joint.JointType == JointType::JointType_ElbowLeft) {
					//���Ђ�
					bodyPoints[8] = c_p;
				}
				else if (joint.JointType == JointType::JointType_HandLeft) {
					//����
					bodyPoints[9] = c_p;

					vector<ofPoint> handRect;
					//��̒��S���獶��
					ofPoint depthLeftUp;
					Pos.X -= range;
					Pos.Y += range;
					DepthSpacePoint tmp, tmp2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp);
					depthLeftUp.x = (int)tmp.X;
					depthLeftUp.y = (int)tmp.Y;
					handRect.push_back(depthLeftUp);
					//��̒��S����E��
					ofPoint depthRightDown;
					Pos.X += range * 2;
					Pos.Y -= range * 2;
					mapper->MapCameraPointToDepthSpace(Pos, &tmp2);
					depthRightDown.x = (int)tmp2.X;
					depthRightDown.y = (int)tmp2.Y;
					handRect.push_back(depthRightDown);
					//�ۑ�
					handPointsLeft.push_back(handRect);

				}

			}
		}
		colorBodyPoints.push_back(bodyPoints);
		depthChange.push_back(false);
	}
}

void ofApp::setColorFromDepth(ofPoint p) {
	//�J���[���W�֕ϊ�
	DepthSpacePoint dPoint;
	ColorSpacePoint cPoint;
	dPoint.X = (float)p.x;
	dPoint.Y = (float)p.y;
	mapper->MapDepthPointToColorSpace(dPoint, depthBuffer[p.y * depthImage.width + p.x], &cPoint);
	//�͈͎w��
	if ((int)cPoint.X < 0 || (int)cPoint.X >= colorWidth ||
		(int)cPoint.Y < 0 || (int)cPoint.Y >= colorHeight)return;
	//RGB�l���擾
	int ptr_rgb = (int)cPoint.Y * colorWidth * 4 + (int)cPoint.X * 4;
	int b = colorPixels[ptr_rgb];
	int g = colorPixels[ptr_rgb + 1];
	int r = colorPixels[ptr_rgb + 2];
	ofSetColor(b, g, r);
}

ofPoint ofApp::getColorFromDepth(ofPoint p) {
	//�J���[���W�֕ϊ�
	DepthSpacePoint dPoint;
	ColorSpacePoint cPoint;
	dPoint.X = (float)p.x;
	dPoint.Y = (float)p.y;
	mapper->MapDepthPointToColorSpace(dPoint, depthBuffer[p.y * depthImage.width + p.x], &cPoint);
	//�͈͎w��
	if ((int)cPoint.X < 0 || (int)cPoint.X >= colorWidth ||
		(int)cPoint.Y < 0 || (int)cPoint.Y >= colorHeight) {
		return ofPoint(0, 0);
	}
	else {
		return ofPoint((int)cPoint.X, (int)cPoint.Y);
	}
}

//--------------------------------------------------------------

//f�����͂�����circle�𔭐�
void ofApp::keyPressed(int key) {
	if (key == 'f') {
		//circle�̍쐬_______________________________________________________________________
		if (circles.size() < 500) {
			//if (frame_count % 2 == 0)continue;
			float ram_size = ofRandom(5, 15);
			float ram_location = ofRandom(100, 1700);
			float ram_weight = ofRandom(10, 50);
			float s_x = colorWidth / 2 + ofRandom(-300, 300);

			circles.push_back(ofPtr<ofxBox2dCircle>(new ofxBox2dCircle));	//ofxBox2dcircle�N���X�̃C���X�^���X����
			circles.back().get()->setPhysics(100, 0.53, 0.1);	//�i�d���A�����́A���C�́j
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
		//��
		if (_r >= 255) {
			move = -move_value;
		}
		else {
			move = +move_value;
		}
	}
	else if (ram == 1) {
		//��
		if (_g >= 255) {
			move = -move_value;
		}
		else {
			move = move_value;
		}
	}
	else {
		//��
		if (_b >= 255) {
			move = -move_value;
		}
		else {
			move = move_value;
		}
	}

	return ofPoint(ram, move);


}