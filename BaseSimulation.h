#ifndef BASESIMULATION_H
#define BASESIMULATION_H

#include <string>
#include <vector>

class SimulationObject {
private:
    std::string name;

public:
    explicit SimulationObject(const std::string& _name) : name(_name) {}
    virtual void update() = 0; // Pure virtual
    virtual void collide() = 0; // Pure virtual
};

class Simulation;

class Configure {
private:
  
    Simulation* simulation;

public:
    Configure(Simulation* simulation = nullptr);
    virtual void configure() = 0; // Pure virtual
    void setSimulation(Simulation* sim) { simulation = sim; }
};

class Clock {
public:
    Clock(int initialTimesteps, int timestepsLimit)
        : timesteps(initialTimesteps), timestepsLimit(timestepsLimit), simulationRunning(true) {}
    int getTimesteps() const { return timesteps; }
    int getTimestepsLimit() const { return timestepsLimit; }
    virtual void update() {
        timesteps++;
        if (timesteps >= timestepsLimit) {
            simulationRunning = false;
        }
    }

    virtual bool checkStop() const { return simulationRunning; }
    

protected:
    int timesteps;
    int timestepsLimit;
    bool simulationRunning;
};

class Simulation {
private:
    std::vector<SimulationObject*> simulationObjects;
    Configure* config;
    Clock* clockPTR;

public:
    Simulation();
    void run() {
        config->configure();
        while (clockPTR->checkStop()){
            size_t numObjects = simulationObjects.size();
            for (size_t i = 0; i < numObjects; ++i) {
                SimulationObject* obj = simulationObjects[i];
                if (obj == nullptr) continue;
                obj->update();
            }
            
            clockPTR->update();
        }
    }
    void setConfig(Configure* cfg);

    void setClock(Clock* clock){
        clockPTR = clock;
    };

    void addObject(SimulationObject* obj) {
        simulationObjects.push_back(obj);
    }
    std::vector<SimulationObject*> getObjects(){
        return simulationObjects;
    }
};

// Inline definitions to resolve linker issues
inline Configure::Configure(Simulation* simulation)
    : simulation(simulation) {}

inline Simulation::Simulation()
    : config(nullptr), clockPTR(nullptr) {}


inline void Simulation::setConfig(Configure* cfg) {
    config = cfg;
    if (config) config->setSimulation(this);
}

#endif