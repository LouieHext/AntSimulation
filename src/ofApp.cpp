#include "ofApp.h"
#include "ofxPS3EyeGrabber.h"


//created by Louie Hext 03/2022

//"interactive CV Ants", 
//A pheromone based ant simulation. Ants strive to explore the map, find food and return to the nest
//phermones guide the ant between their targets. The quicker the path the more frequently the
//pheromones build up, and the more ants that will be attracted to it. This naturally optimises paths

//to interac with the ants you can use coloured paper below a webcam. The paper is directly fed
//into the simulation, allowing you to act with the ants
//--------------------------------------------------------------
void ofApp::setup() {
	ofEnableAlphaBlending();
	setupParams();										//setting up GUI and shader uniforms
	//setupWebcam();										//setting up webcam
	setupAnts();										//setting up ant structs
	setupShaders();										//setting up buffers and textures
	
}

//--------------------------------------------------------------
void ofApp::update() {
	
	updateAnts();										//update the ants using the simulation shader
	updatePheromones();									//update the pheromones using the diffusion shader
	

}

//--------------------------------------------------------------
void ofApp::draw(){
	
	showFPS();											//displaying fps
	ofSetColor(255,255,255,255);						//setting no tint
	display.draw(0, 0,ofGetWidth(),ofGetHeight());		//drawing texture from shader
	displayAlt.draw(0, 0, ofGetWidth(), ofGetHeight());
	
	
	gui.draw();											//drawing GUI
	

	
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if (key == 'r') {									//if reseting
		antsBufferClear.copyTo(antsBuffer);				//copy default ant struct ant buffer
		pheremonesClear.copyTo(pheremonesToFood);		//copy emty array to pheromones
		pheremonesClear.copyTo(pheremonesToFoodBack);
		pheremonesClear.copyTo(pheremonesToNest);
		pheremonesClear.copyTo(pheremonesToNestBack);
		//generateMap();									//load up random noise based generation 
		foodBuffer.allocate(W*H * sizeof(float) * 3, foodCPU, GL_STATIC_DRAW);
		foodBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);
		foodBufferClear.copyTo(foodBufferBack);
		foodBufferClear.copyTo(foodBufferCopy);
	}

	if (key == 'g') {									//if generating a new map
		generateMap();									//load up random noise based generation 
														//reallocare storage for map infomation
		foodBuffer.allocate(W*H * sizeof(float) * 3, foodCPU, GL_STATIC_DRAW);
		foodBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);

		antsBufferClear.copyTo(antsBuffer);				//copy default ant struct ant buffer
		pheremonesClear.copyTo(pheremonesToFood);	    //copy emty array to pheromones
		pheremonesClear.copyTo(pheremonesToFoodBack);
		pheremonesClear.copyTo(pheremonesToNest);
		pheremonesClear.copyTo(pheremonesToNestBack);
	}

}

//generates a random map with walls, food, and nest
//the walls are generated using a random seed and with noise to give variation
//the food and nests are always in the same place for ease of use
//(nest values are assumed in the ant set up function)
void ofApp::generateMap() {
	int seed = ofRandom(0, 1000);									//random seed
	for (int x = 0; x < W; x++) {									//looping over all pixels
		for (int y = 0; y < H; y++) {
			int idx = x + y * W;									//2D->1D
																	//modifying x and y with noise


			float xx = x + W * 0.04*ofSignedNoise(seed + 100 + x * bumpiness, seed + 200 + y * bumpiness);
			float yy = y + H * 0.04*ofSignedNoise(seed + 300 + x * bumpiness, seed + 500 + y * bumpiness);

			pheremonesCPU[idx] = 0.0;								//setting all values to zero as default
			foodCPU[idx] = 0.0;
			foodCPU[idx + W * H] = 0.0;
			foodCPU[idx + 2*W * H] = 0.0;
			foodCPUClear[idx] = 0.0;
			foodCPUClear[idx + W * H] = 0.0;
			foodCPUClear[idx + 2 * W * H] = 0.0;

			if (ofDist(xx, yy, W*0.2, H*0.2) < W*0.1) {				//adding some food in top left
				foodCPU[idx] = 100;									//food is positive
			}
			if (ofDist(xx, yy, W*0.6, H*0.3) < W*0.05) {			//adding some food in top right
				foodCPU[idx] = 100;									//food is positive
			}
			if (ofNoise(seed + 0.005*xx*ofMap(ofDist(xx,yy,W*0.5,H*0.5),0,W,1,0.1), seed + 0.005*yy*ofMap(ofDist(xx, yy, W*0.5, H*0.5), 0, W, 1, 0.1)) > 0.7) {	//adding in noise based walls
				foodCPU[idx] = -10000;								//walls are negative values
			}
			int gap = W * 0.05;										//defining border wall size
			if (xx < gap || yy < gap || xx>W - gap || yy>H - gap) { //making it noisey
				foodCPU[idx] = -10000;
			}
			if (ofDist(x, y, W*0.8, H*0.8) < W*0.025) {				//adding circular nest in lower right
				foodCPU[idx + W * H] = 1000;
			}
			if (ofDist(x, y, W*0.2, H*0.7) < W*0.025) {				//adding in circular nest in lower left
				foodCPU[idx + W * H] = 1000;
			}
		}
	}
}


//populates the ant vector with ant structs.
//ants will spawn near their nest with random headings
//two additional data points are added to help with GPU alignment but arent
//actually needed.
void ofApp::setupAnts() {
	ants.resize(1024 * 128);						//resizing ant vector
	
	glm::vec2	nest = glm::vec2(W*0.8, H*0.8);				//default position in lower right
	

	for (auto & ant : ants) {						//random position around nest
		ant.pos = nest + glm::vec2(ofRandom(-5, 5), ofRandom(-5, 5));
		ant.heading = ofRandom(0, 2 * PI);			//random heading
		ant.food = false;							//start with no food
		ant.time = 1.0;								//time since last key location, avoid starting at 0 for 1/0 .
													//setting previous position to same start
		glm::vec2 prevPos = nest+ glm::vec2(ofRandom(-5, 5), ofRandom(-5, 5));
		ant.alignFix = 0;							// extra  bites to fit alignment (see header)
		ant.alignFixx = 0.0;
	}

}


//sets up everything needed by the shader pipeline
//links compute shaders
//allocates buffers and textures
//binds buffers and textures to the GPU
void ofApp::setupShaders() {

	//loading in shader files and linking them
	loader.setupShaderFromFile(GL_COMPUTE_SHADER, "loader.glsl");          //loads in map infomation from webcam
	loader.linkProgram();
	simulation.setupShaderFromFile(GL_COMPUTE_SHADER, "simulation.glsl");  //simulates ant movement
	simulation.linkProgram(); 
	diffusion.setupShaderFromFile(GL_COMPUTE_SHADER, "Diffusion.glsl");    //diffuses pheromones
	diffusion.linkProgram();

	setupAnts();														   //fills up ant vector with ant structs

	generateMap();														   //generates map (food,wall,nest) infomation using noise
	

	//allocating GPU buffer objects
	pheremonesToFood.allocate(W*H * sizeof(float), pheremonesCPU, GL_STATIC_DRAW);		//ant pheromones to food
	pheremonesToFoodBack.allocate(W*H * sizeof(float), pheremonesCPU, GL_STATIC_DRAW);  //for "instantaneous" global update

	pheremonesToNest.allocate(W*H * sizeof(float), pheremonesCPU, GL_STATIC_DRAW);		//ant pheromones to nest
	pheremonesToNestBack.allocate(W*H * sizeof(float), pheremonesCPU, GL_STATIC_DRAW);  //for "instantaneous" global update

	pheremonesClear.allocate(W*H * sizeof(float), pheremonesCPU, GL_STATIC_DRAW);		//used to clear arrays to default 0.0

	foodBuffer.allocate(W*H * sizeof(float) * 3, foodCPU, GL_STATIC_DRAW);			//stores map infomation in "layers" [food, nests, particle positions]
	foodBufferBack.allocate(W*H * sizeof(float) * 3, foodCPU, GL_STATIC_DRAW);		//for "instantaneous" global update
	foodBufferCopy.allocate(W*H * sizeof(float) * 3, foodCPU, GL_STATIC_DRAW);
	foodBufferClear.allocate(W*H * sizeof(float) * 3, foodCPUClear, GL_STATIC_DRAW);
	antsBuffer.allocate(ants, GL_DYNAMIC_DRAW);											//buffer of ant structs
	antsBufferClear.allocate(ants, GL_STATIC_DRAW);										//holds initial ants for easy reset



	//binding buffers so GPU knows whats what (GPU wont know variable names)
	antsBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 0);
	pheremonesToFood.bindBase(GL_SHADER_STORAGE_BUFFER, 1);
	pheremonesToNest.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
	pheremonesToFoodBack.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
	pheremonesToNestBack.bindBase(GL_SHADER_STORAGE_BUFFER, 4);
	foodBuffer.bindBase(GL_SHADER_STORAGE_BUFFER, 5);
	foodBufferBack.bindBase(GL_SHADER_STORAGE_BUFFER, 8);								//image binds cant be last 

	//allocating and binding display texture which we use to visualise GPU results
	displayAlt.allocate(W, H, GL_RGBA8);
	displayAlt.bindAsImage(6, GL_WRITE_ONLY);											//having two displays for different pheromones
	display.allocate(W, H, GL_RGBA8);													//allows for alpha blending
	display.bindAsImage(7, GL_WRITE_ONLY);
	
}

//sets up the paramters and the GUI which control the simulation and the webcam
//split into "ants", "heromnes" and "calibration"
void ofApp::setupParams() {

	//setting up GUI and simulation params
	gui.setup();
	//setting up ant params
	antSettings.setName("Ant params");
	antSettings.add(maxSpeed.set("maxSpeed", 5.8, 0, 20)); 					  //step size of ants
	antSettings.add(turningSpeed.set("turningSpeed", 0.7, 0, 3.141));		  //angular step size
	antSettings.add(sensorAngle.set("sensorAngle", 0.37, 0, 3.141));		  //angle at which ants checks phermones
	antSettings.add(sensorDistance.set("sensorDistance", 18, 1, 25));		  //distance at when ant checks phermones
	antSettings.add(sensorSize.set("sensorSize", 1, 0, 5));					  //kernel size of pheremone check
	antSettings.add(foodDesire.set("foodDesire", 10, 0, 10));				  
	//setting up pheromone params											  
	pheromoneSettings.setName("Pheromone params");							  
	pheromoneSettings.add(decayWeight.set("decayWeight", 0.18, 0, 1));		  //value at which all pheromones decay
	pheromoneSettings.add(diffusionWeight.set("diffusionWeight", 0.7, 0, 1)); //value at which all pheromones diffuse
	
	mapSettings.setName("Map params");
	mapSettings.add(bumpiness.set("bumpiness", 0.001, 0.00001, 0.01));
	//adding to GUI
	gui.add(antSettings);
	gui.add(pheromoneSettings);
	gui.add(mapSettings);

}

//dispatched the simulation compute shader to update the ants
void ofApp::updateAnts() {
	simulation.begin();												//starting shader
	simulation.setUniforms(antSettings);							//ant param uniform
	simulation.setUniform1f("time", ofGetFrameNum()*0.1);			//sending "time"
	simulation.setUniform1i("W", W);								//sending resolution
	simulation.setUniform1i("H", H);
	simulation.dispatchCompute(1024, 1, 1);							//setting 1024 work groups for parallelisation
	simulation.end();												//ending shader
}

//dispatched the diffusion compute shader to update the pheromones
void ofApp::updatePheromones() {
	diffusion.begin();												//starting shader
	diffusion.setUniforms(pheromoneSettings);						//pheromone param uniform
	diffusion.setUniform1f("time", ofGetFrameNum()*0.1);;			//sending "time"
	diffusion.setUniform1i("W", W);									//sending resolution
	diffusion.setUniform1i("H", H);
	diffusion.dispatchCompute(W / 20, H / 20, 1);					//sending workgroups assuming local group size of 20*20
	diffusion.end();												//ending shader

	pheremonesToFoodBack.copyTo(pheremonesToFood);					//completing "instantenous" global updates
	pheremonesToNestBack.copyTo(pheremonesToNest);
	pheremonesClear.copyTo(antPos);
}



//helper function to show FPS in window title
void ofApp::showFPS() {
	ofSetColor(255);												//setting white
	std::stringstream strm;
	strm << "fps: " << ofGetFrameRate();							//frame rate to sting stream
	ofSetWindowTitle(strm.str());
}