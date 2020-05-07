#include "trafficSimulator.h"


/* trafficSimulator
 * input: char* name of file to read
 * output: N/A
 *
 * Simulate the road network in the given file
 */
void trafficSimulator( char * filename )
{
    /* Read in the traffic data.  You may want to create a separate function to do this. */
    TrafficSimData *simulationData = readTrafficData(filename);

    /* Loop until all events processed and either all cars reached destination or gridlock has occurred */
    for(int cycle = 0; ; cycle++) {
        //pop out of PQ and place in queue of cars to be added to road

        /* Loop on events associated with this time step */
        while(!isEmptyPQ(simulationData->events) && cycle == getFrontPriority(simulationData->events)) {
            //events ahve from/to
            Event * event = dequeuePQ(simulationData->events);
            RoadData * roadData = getEdgeData(simulationData->trafficGraph, event->starting, event->ending);
            Car * car = malloc(sizeof(Car));
            car->destination = event->destinationVertex;
            car->totalTimeSteps = 0;
            enqueue(roadData->waitingToEnter, car);
            
            /* have we already enqueued a car this cycle */
            // if(roadData->lastTimeCarsEnqueued != cycle) {
            //     printf("CYCLE %d - ADD_CAR_EVENT - Cars enqueued on road from %d to %d\n", cycle, roadData->from, roadData->to);
            //     roadData->lastTimeCarsEnqueued = cycle;
            // }
        }

        /* print road events */
        printf("CYCLE %d - PRINT_ROADS_EVENT - Current contents of the roads:\n", cycle); 
        int i, j;
        for(i = 0; i < simulationData->numRoads; i++) {
            RoadData * currentRoad = simulationData->roads[i];
            printf("Cars on the road from %d to %d:\n", currentRoad->from, currentRoad->to);
            for(j = 0; j < currentRoad->lengthOfRoad; j++) {
                if(currentRoad->carsOnRoad[j] == -999) {
                    printf("- ");
                }
                else {
                    printf("%d ", currentRoad->carsOnRoad[j]);
                }
            }
            printf("\n");
        }
        printf("\n\n");


        /* First try to move through the intersection */
        for(i = 0; i < simulationData->numRoads; i++) {
            RoadData * currentRoad = simulationData->roads[i];
            int carToMove = currentRoad->carsOnRoad[0];

            if(carToMove == -999)
                continue;

            //if the light is greeen
            if(currentRoad->currentCycle >= currentRoad->greenOn && currentRoad->currentCycle < currentRoad->greenOff) {
                // check cars destination && check shortest route
                if(currentRoad->to == carToMove) {
                    currentRoad->carsOnRoad[0] = -999;
                    // car reached destination
                    printf("CYCLE %d - Car successfully traveled from N/A to N/A in N/A time steps.\n", cycle);

                    simulationData->currentCarsActive--;
                }
                else {
                    currentRoad->carsOnRoad[0] = -999;

                    int * pNext = NULL;
                    if(!getNextOnShortestPath(simulationData->trafficGraph, currentRoad->to, carToMove, pNext)) {
                        break;
                    }

                    RoadData * nextRoad = getEdgeData(simulationData->trafficGraph, currentRoad->to, pNext);

                    Car * car = malloc(sizeof(Car));
                    car->destination = carToMove;
                    // car->totalTimeSteps++;
                    enqueue(nextRoad->waitingToEnter, car);
                }
            }
        }
        
        /* Second move cars forward on every road */
        for(i = 0; i < simulationData->numRoads; i++) {
            RoadData * currentRoad = simulationData->roads[i];
            int * carsOnRoad = currentRoad->carsOnRoad;

            for(j = currentRoad->lengthOfRoad - 1; j > 0; j--) {
                if(carsOnRoad[j] != -999 && carsOnRoad[j - 1] == -999) {
                    carsOnRoad[j - 1] = carsOnRoad[j];
                    carsOnRoad[j] = -999;
                    j--;
                }
            }
        }

        /* Third move cars onto road if possible */
        for(i = 0; i < simulationData->numRoads; i++) {
            RoadData * currentRoad = simulationData->roads[i];
            int * cars = currentRoad->carsOnRoad;
            int lengthOfRoad = currentRoad->lengthOfRoad;

            if(cars[lengthOfRoad - 1] == -999 && !isEmpty(currentRoad->waitingToEnter)) {
                // move waiting car onto road
                Car * car = dequeue(currentRoad->waitingToEnter);
                currentRoad->carsOnRoad[lengthOfRoad - 1] = car->destination;
            }
        }

        /* change light cycles */
        for(i = 0; i < simulationData->numRoads; i++) {
            RoadData * currentRoad = simulationData->roads[i];

            currentRoad->currentCycle++;
            
            if(currentRoad->currentCycle == currentRoad->resetCycle)
                currentRoad->currentCycle = 0;
        }

        /* check if current moving cars are zero */
        if(simulationData->currentCarsActive <= 0)
            break;
    }

    /* final print statements */
    // Average number of time steps to the reach their destination is 4.00.
    // Maximum number of time steps to the reach their destination is 4.
}

TrafficSimData *readTrafficData(char* filename) {
    TrafficSimData *simulationData = malloc(sizeof(TrafficSimData));

    FILE * fp;
    fp = fopen(filename, "r");

    int i, j, k;
    
    int verticies, edges;
    fscanf(fp, "%d", &verticies);
    fscanf(fp, "%d", &edges);
    
    simulationData->trafficGraph = createGraph(verticies);
    simulationData->events = createPQ();
    simulationData->roads = malloc(sizeof(RoadData) * edges);
    simulationData->numRoads = edges;
    int roadCounter = 0;

    for(i = 0; i < verticies; i++) {
        int toVertex, incomingRoads;
        fscanf(fp, "%d", &toVertex);
        fscanf(fp, "%d", &incomingRoads);

        if(!isVertex(simulationData->trafficGraph, toVertex))
            addVertex(simulationData->trafficGraph, toVertex);

        for(j = 0; j < incomingRoads; j++) {
            int fromVertex, lengthOfRoad, greenOn, greenOff, resetCycle;
            // scan in fromVertex & lengthOfEdge
            fscanf(fp, "%d", &fromVertex);
            fscanf(fp, "%d", &lengthOfRoad);

            // greenOn to i vertex
            fscanf(fp, "%d", &greenOn);
            // greenOff to i vertex
            fscanf(fp, "%d", &greenOff);
            // resetCycle to i vertex
            fscanf(fp, "%d", &resetCycle);

            RoadData *roadDataStruct = malloc(sizeof(RoadData));
            roadDataStruct->to = toVertex;
            roadDataStruct->from = fromVertex;
            roadDataStruct->lengthOfRoad = lengthOfRoad;
            roadDataStruct->greenOn = greenOn;
            roadDataStruct->greenOff = greenOff;
            roadDataStruct->resetCycle = resetCycle;
            roadDataStruct->lastTimeCarsEnqueued = -1;
            roadDataStruct->carsOnRoad = malloc(sizeof(int) * lengthOfRoad);
            for(k = 0; k < lengthOfRoad; ++k)
                roadDataStruct->carsOnRoad[k] = -999;

            roadDataStruct->waitingToEnter = createQueue();
            
            setEdge(simulationData->trafficGraph, fromVertex, toVertex, lengthOfRoad);
            setEdgeData(simulationData->trafficGraph, fromVertex, toVertex, roadDataStruct);
            simulationData->roads[roadCounter++] = roadDataStruct;
        }
    }



    int addCarCommands;
    fscanf(fp, "%d", &addCarCommands);
    for(i = 0; i < addCarCommands; i++) {
        int starting, ending, whenToAddCar, howManyToAdd;
        
        fscanf(fp, "%d", &starting);
        fscanf(fp, "%d", &ending);
        fscanf(fp, "%d", &whenToAddCar);
        fscanf(fp, "%d", &howManyToAdd);

        for(j = 0; i < howManyToAdd; i++) {
            // adding into an array?
            int destinationVertex;
            fscanf(fp, "%d", &destinationVertex);

            Event *eventData = malloc(sizeof(Event));
            eventData->starting = starting;
            eventData->ending = ending;
            eventData->whenToAddCar = whenToAddCar;
            eventData->destinationVertex = destinationVertex;

            enqueueByPriority(simulationData->events, eventData, whenToAddCar);
            simulationData->currentCarsActive++;
        }
    }

    int printRoadCommands;
    fscanf(fp, "%d", &printRoadCommands);
    simulationData->printCommandSteps = malloc(sizeof(int) * printRoadCommands);
    for(i = 0; i < printRoadCommands - 1; i++) {
        int cycleToPrintRoads;
        fscanf(fp, "%d", &cycleToPrintRoads);
        simulationData->printCommandSteps[i] = cycleToPrintRoads;
    }
    fclose(fp);
    return simulationData;
}