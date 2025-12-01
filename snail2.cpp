#include "BaseSimulation.h" // for base classes
#include <algorithm>  // for std::shuffle, std::sort
#include <cstdlib>    // for rand, RAND_MAX
#include <ctime>      // for time
#include <fstream>    // for file I/O
#include "json.hpp"   // for JSON
#include <iostream>   // for console output
#include <stdexcept>  // for exceptions
#include <string>     // for std::string
#include <vector>     // for std::vector

using json = nlohmann::json;

struct Point {
    int x;
    int y;

    // Inline constructor definition, added because of errors with construction
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};
struct taggedPoint{
    std::string nameTag;
    int timeTag;
    Point point;
};

// snailData for CSV output
struct RegionData{
    int foodLevel;
    int numOfSnails;

};
struct RegionsData{
    int time;
    int totalPop;
    std::vector<RegionData> regions;
};

class Region: public SimulationObject {
    private:
        int halfLength;
        Point centerPoint;
        int foodGrowth;
        int regionFood;
        int maxFood; 

    public:
        Region(std::string name, int pPredProb, int phalfLength,Point pCenterPoint, int pFoodPercentage,int pTotalFood, int pFoodGrowth, int pMaxFood):
        SimulationObject(name),halfLength(phalfLength), centerPoint(pCenterPoint),regionFood((pTotalFood * pFoodPercentage)/100), foodGrowth((pFoodGrowth*pFoodPercentage)/100), maxFood((pMaxFood*pFoodPercentage)/100){}

        Point getRegionPos(){return centerPoint;};
        int getRegionhalfLength(){return halfLength;}
        int getFoodLevel(){return regionFood;};
        void collide()override{}
        void update() override {
            regionFood = std::min(regionFood += foodGrowth, maxFood);
        }
        int getFood(int foodAmount){
            if (regionFood>= foodAmount){
                regionFood -= foodAmount;
                return foodAmount;
            }
            return 0;   
        };

        
};
//SwampClock
class SwampClock : public Clock{
public:
    SwampClock(int initialTimesteps, int timestepsLimit):
        Clock( initialTimesteps, timestepsLimit),
         timesteps(initialTimesteps), timestepsLimit(timestepsLimit), simulationRunning(true){}
    int getTimesteps() const { return timesteps; }
    int getTimestepsLimit() const { return timestepsLimit; }
    virtual void update() {
        timesteps++;
        if (timesteps >= timestepsLimit) {
            simulationRunning = false;
        }
    }

    virtual bool checkStop() const { return simulationRunning; }
    void setStop() {
        simulationRunning = false;
    }

protected:
    int timesteps;
    int timestepsLimit;
    bool simulationRunning;
};
//Swamp Class
class Swamp: public SimulationObject {
private:

    int width;
    int length;
    Point midPoint = {0,0};
    Simulation* sim;
    std::vector<Region*> regions;

public:
    Swamp(const std::string& name ,int swampFoodRegen, int swampMaxFood, int swampInitialFood, Simulation* simulation, int swampWidth, int swampLength)
    : SimulationObject(name), sim(simulation),width(swampWidth),length(swampLength){}
    void update() override {}  
   
    Region* getRegion(int i ){return regions[i];}
    void setRegions(std::vector<Region*> pRegion){regions = pRegion;}

    void collide()override{}

    int checkRegion(Point posToCheck, Region*  regionToCheck){
        int halfLength = regionToCheck->getRegionhalfLength();
        Point centerPoint = regionToCheck->getRegionPos();
        if ((std::abs(posToCheck.x-centerPoint.x)<=halfLength) && (std::abs(posToCheck.y-centerPoint.y)<=halfLength)){ // regions are squares; width == length 
            return 1;
        }
        return 0;
    }
    int getRegionInt(Point pos){
        int i = 0;
        for (Region* region : regions) {
            if (checkRegion(pos, region) == 1){
                return i;
            }
            i+=1;
        } 
        return -1; // invalid result of -1 to cause error     
    
    }
    void addSnail(SimulationObject* SimObj){
        sim->addObject(SimObj);
    }
    Point checkPos(Point pos){
        if (pos.x > width){
            pos.x = width;
        }
        if (-1*pos.x > width){
            pos.x = -1*width;
        }
        if (pos.y > length){
            pos.y = length;
        }
        if (-1*pos.y > length){
            pos.y = -1*length;
        }
        return pos;
    }
};

//Snail Class
class Snail : public SimulationObject {
private:
    std::string name;
    int age;
    bool isAlive;
    int reproProb;
    int predProb;
    int maturityAge;
    int maxAge;
    int minOffspring;
    int maxOffspring;
    int daysStarved = 0;
    int mealSize = age*0.1; // intialize mealSize in accordance to age
    int healthIndex = 3;
    bool eatenStatus = false;
    Point pos;
    Swamp& swamp;
    int regionInt;
    Region* region;

public:
    Snail(const std::string pName, int Age,int snailReproProb, int snailPredProb, int snailMaturityAge, int snailMaxAge, int snailMinOffspring,int snailMaxOffspring,Point Position,Swamp& swamp)
        : SimulationObject(name), name(pName),age(Age), isAlive(true), reproProb(snailReproProb),predProb(snailPredProb), maturityAge(snailMaturityAge), maxAge(snailMaxAge), minOffspring(snailMinOffspring),maxOffspring(snailMaxOffspring),pos(Position),swamp(swamp){}

    bool getAliveStatus() const { return isAlive;};
    int getRegionNum () {return regionInt;};
    void setEatenStatus(bool status){eatenStatus = status;}
    Point getPos(){return pos;};
    const std::string& getName(){return name;};
    void collide()override{}
    void reproduce() {
            int numOffspring = minOffspring + (rand() % ((healthIndex*maxOffspring) - (healthIndex*minOffspring) + 1));
            for (int i = 0; i < numOffspring; i++) {
                std::string newName = name + " o" + std::to_string(i);
                Snail* offspring = new Snail(
                    newName, 0,reproProb, predProb, maturityAge, maxAge,
                    minOffspring, maxOffspring, pos, swamp
                );
                swamp.addSnail(offspring);
            
        }
    }

    void update() override {
        regionInt = swamp.getRegionInt(pos);
        region = swamp.getRegion(regionInt);

        
        if (isAlive == false){
            return;
        }
        move();
        age++;
        int tMealhalfLength = (age*0.1);
        mealSize = std::min(20,tMealhalfLength);
        if (age > maxAge){
            isAlive = false;
            return;
        }
        int foodAmount = region->getFood(mealSize);
        if (foodAmount == 0) {
                daysStarved += 1;
                healthIndex = std::max(healthIndex-1,1); 
                if (daysStarved > 10){
                    isAlive = false;
                    return;
                }
        }
        else {
            daysStarved = 0;
            if (foodAmount>=mealSize){
                healthIndex = std::min(healthIndex+1,5);
            }
            else{
                healthIndex = std::min(healthIndex+(mealSize/foodAmount),5);
            }
        }
        if (eatenStatus == true){
            isAlive = false; 
            return;
        }
        int rNum = (rand() % reproProb);
        if (age > maturityAge && rNum == 0) {
            reproduce();
        }
    }
    void move() { //move randomly and then check for boundaries 
        pos.x += (rand() % 3) - 1;
        pos.y += (rand() % 3) - 1;
        pos = swamp.checkPos(pos);
    }
};
class Predator : public SimulationObject{
    private:
        std::string name;
        int predProb;
        Point positon;
        int maxAge;
        Simulation* sim;
        Swamp* swamp;
        bool hungryStatus = false;
    public:
    Predator(std::string pName, int pPredProb, Point pPosition, int pMaxAge, Simulation* pSim, Swamp* pSwamp)
        : SimulationObject(name), predProb(pPredProb), positon(pPosition), maxAge(pMaxAge), sim(pSim),swamp(pSwamp){}
    void collide()override{}
    void update()override{
        std::vector<SimulationObject*> simObjects = sim->getObjects();
        for (SimulationObject* obj : simObjects){
            if (Snail* snail = dynamic_cast<Snail*>(obj)) {
                bool isAlive = snail->getAliveStatus();
                    if (isAlive){ // check if obj is a snail and is alive
                        Point snailPos = snail->getPos();
                        int regionNum = swamp->getRegionInt(snailPos);
                        Region* region = swamp->getRegion(regionNum);
                        int halfLength = region->getRegionhalfLength();
                        if ((std::abs(snailPos.x-positon.x)<=halfLength) && (std::abs(snailPos.y-positon.y)<=halfLength)){ // check if snail is within same region as Pred
                            int num = (rand() % predProb);
                            if (num == 0){
                                snail->setEatenStatus(true);
                                hungryStatus = true;

                        }
                    }
                }
            }
        }
    }
};     
class DataCollector : public SimulationObject{
    private:
        std::string name;
        Swamp* swamp;
        SwampClock* clock;
        Simulation* world;
        std::vector<taggedPoint> snailPositons;
    public:
        DataCollector(const std::string& name, SwampClock* Clock, Simulation* sim)
        : SimulationObject(name),clock(Clock), world(sim){}

        std::vector<RegionsData> outputData;
        void collide()override{}
        void update()override {
            RegionsData newEntry = RegionsData{};
            int timestep = clock->getTimesteps();
            newEntry.time = timestep;
            std::vector<SimulationObject*> simObjects = world->getObjects();
            for (SimulationObject* obj : simObjects) {
                if (Region* region = dynamic_cast<Region*>(obj)) {
                    int foodLevel = (region->getFoodLevel());
                    RegionData regionData = {foodLevel,0};
                    newEntry.regions.push_back(regionData);
                    
                }

                else if (Snail* snail = dynamic_cast<Snail*>(obj)) {
                    bool isAlive = snail->getAliveStatus();
                    if (isAlive){
                        int regionNum = snail->getRegionNum();
                        newEntry.regions[regionNum].numOfSnails +=1;
                        newEntry.totalPop += 1;
                        Point pos = snail->getPos();
                        std::string snailName = snail->getName();
                        taggedPoint snailPos = {snailName, timestep, pos};
                        snailPositons.push_back(snailPos);

                    }

                }
            
            
            
            }

            outputData.push_back(newEntry);
        }
         std::vector<RegionsData>returnOutputData(){return outputData;}
        std::vector<taggedPoint>returnPositions(){return snailPositons;}
        
};

// SwampConfig Class
class SwampConfig : public Configure {
private:
    DataCollector* Collector;
    std::string configFilePath;
    Simulation* simulation;
    SwampClock* clock;
    int snailCount;
    int duration;
    int swampFoodRegen;
    int swampMaxFood;
    int swampInitialFood;
    int swampWidth;
    int swampLength;
    int snailReproProb;
    int snailPredProb;
    int snailMaturityAge;
    int snailMaxAge;
    int snailMinOffspring;
    int snailMaxOffspring;

    
public:
    SwampConfig(const std::string& configFilePath, int snails, int duration, SwampClock* Clock, int pReproProb, int pPredProb) // snails and duration are command line parameters
        : Configure(nullptr), 
          configFilePath(configFilePath), snailCount(snails), duration(duration),clock(Clock), snailPredProb(pPredProb), snailReproProb(pReproProb){}

    std::vector<RegionsData> data;
    std::vector<taggedPoint> positionSummary;

    void readJson() {
        std::ifstream file(configFilePath);
        if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + configFilePath);
        }
        json j;
        file >> j;
        swampFoodRegen = j["foodRegen"].get<int>();
        swampMaxFood = j["maxFood"].get<int>();
        swampInitialFood = j["initialFood"].get<int>();
        swampWidth = j["swampWidth"].get<int>();
        swampLength = j["swampLength"].get<int>();
        snailMaturityAge = j["maturityAge"].get<int>();
        snailMaxAge = j["maxAge"].get<int>();
        snailMinOffspring = j["minOffspring"].get<int>();
        snailMaxOffspring = j["maxOffspring"].get<int>();
        
    };
    
    
    void configure() override {
        readJson();
        Swamp* swamp = new Swamp("Swamp", swampFoodRegen, swampMaxFood, swampInitialFood, simulation, swampWidth, swampLength);
        
        Point centerPoint = Point(125,-125);
        Region* region1 = new Region("region1",75,125,centerPoint,30,swampInitialFood,swampFoodRegen,swampMaxFood); // name, int pPredProb, int phalfLength,Point pCenterPoint, int pFoodPercentage,int pTotalFood, int pFoodGrowth
        simulation->addObject(region1);
        centerPoint = Point(125,125);
        Region* region2 = new Region("region2",25,125,centerPoint,20,swampInitialFood,swampFoodRegen,swampMaxFood);
        simulation->addObject(region2);
        centerPoint = Point(-125,125);
        Region* region3 = new Region("region3",75,125,centerPoint,30,swampInitialFood,swampFoodRegen,swampMaxFood);
        simulation->addObject(region3);
        centerPoint = Point(-125,-125);
        Region* region4 = new Region("region4",25,125,centerPoint,20,swampInitialFood,swampFoodRegen,swampMaxFood);
        simulation->addObject(region4);
        std::vector<Region*> regions;
        regions.push_back(region1);
        regions.push_back(region2);
        regions.push_back(region3);
        regions.push_back(region4);
        swamp->setRegions(regions);
        Point predPoint = Point(125,125);
        Collector = new DataCollector("Collector",clock, simulation); 
        std::string predName = "pred1";
        Predator* predator = new Predator(predName,50,predPoint,200,simulation,swamp);
        for (int i = 0; i < snailCount;i++){
            std::string name = "Snail" + std::to_string(i);
            int xPos = std::rand() % (2 * swampWidth + 1) - swampWidth; 
            int yPos = std::rand() % (2 * swampLength + 1) - swampLength;
            Point startPos(xPos,yPos);
            Snail* snail = new Snail(
            name,
            (rand() % snailMaxAge),
            snailReproProb,
            snailPredProb,
            snailMaturityAge,
            snailMaxAge,
            snailMinOffspring,
            snailMaxOffspring,
            startPos,
            *swamp
        );
            simulation->addObject(snail);
        }
        simulation->addObject(Collector);
    };
    void setSimulation(Simulation* sim) { simulation = sim; }

    void setData(){
        data = Collector->returnOutputData();
        positionSummary = Collector->returnPositions();
    }

};

class CSVWriter {
private:
    std::string csvFilePath_;
    std::string posFilePath;
    int predProb;
    int reproProb;
public:
    CSVWriter(int predProb, int reproProb, const std::string& csvFilePath, const std::string& posFilePath)
        : csvFilePath_(csvFilePath), posFilePath(posFilePath), predProb(predProb), reproProb(reproProb) {}

    void createCSV(const std::vector<RegionsData> data, const std::vector<taggedPoint>& positions) {
        // Check if CSV file exists
        bool csvExists = false;
        {
            std::ifstream checkCsv(csvFilePath_);
            csvExists = checkCsv.good();
        }

        std::ofstream mainFile(csvFilePath_, std::ios::app);
        if (!mainFile.is_open()) {
            throw std::runtime_error("Failed to open CSV file: " + csvFilePath_);
        }

        if (!csvExists) {
            mainFile << "PredProb, ReproProb, Time, Number Of Snails\n";
        }
        int largestPop = 0;
        int largestPopI = 0;
        for (int aV = 0; aV < data.size();aV++){
            if (data[aV].totalPop > largestPop){
                largestPop = data[aV].totalPop;
                largestPopI = aV;
            }
        }

        mainFile << predProb << "," << reproProb << "," << data[largestPopI].time << "," << data[largestPopI].totalPop << "\n";

        mainFile.close();

    
    }
};
//First argument is number of snails, second is the length of the sim
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <snails> <simulationDuration>\n";
        return 1;
    }

    int snails = std::stoi(argv[1]);
    if (snails <= 0 || snails >= 1000) {
        std::cerr << "Bad choice for number of snails.\n";
        return 1;
    }

    int duration = std::stoi(argv[2]);
    if (duration <= 0 || duration >= 1000) {
        std::cerr << "Bad choice for duration.\n";
        return 1;
    }


    int startReproProb = 10;
    int maxReproProb = 110;
    int reproProbIncrement = 1;
    int startPredProb = 25 ;
    int maxPredProb = 125 ;
    int predProbIncrement = 1;

    std::string configFile = "snailSim2.json";

// Create objects and set dependencies
    srand(time(0));
    for (int reproProb = startReproProb; reproProb < maxReproProb;reproProb+=reproProbIncrement){
        for (int predProb = startPredProb; predProb < maxPredProb;predProb+=predProbIncrement){
        SwampClock* clock = new SwampClock(0, duration);
    
        SwampConfig* swc = new SwampConfig(configFile, snails, duration, clock, reproProb, predProb);
    
        Simulation* world = new Simulation();
    
        // Set up relationships between objects
        world->setClock(clock);
        world->setConfig(swc);
        swc->setSimulation(world);
    
        // Run the simulation
        world->run();
    
        // Dynamically allocate CSVWriter
        CSVWriter* csv_writer = new CSVWriter(predProb, reproProb, "snail2_data.csv","snail2pos_data.csv");
    
        // Set data and create CSV
        swc->setData();
        csv_writer->createCSV(swc->data,swc->positionSummary);
    
        // Clean up dynamically allocated memory
        delete csv_writer;
        delete world;
        delete swc;
        delete clock;

    }
    }
    std::cout << "No Errors ;). Output is at " "snail2_data.csv" " and " "snail2pos_data.csv" "\n";
    return 0;




   

}

