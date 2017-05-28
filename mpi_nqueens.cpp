/**
 * @file    mpi_nqueens.cpp
 * @author  Patrick Flick <patrick.flick@gmail.com>
 * @brief   Implements the parallel, master-worker nqueens solver.
 *
 * Copyright (c) 2016 Georgia Institute of Technology. All Rights Reserved.
 */

/*********************************************************************
 *                  Implement your solutions here!                   *
 *********************************************************************/

#include "mpi_nqueens.h"

#include <mpi.h>
#include <vector>
#include "nqueens.h"

/**
 * @brief The master's call back function for each found solution.
 *
 * This is the callback function for the master process, that is called
 * from within the nqueens solver, whenever a valid solution of level
 * `k` is found.
 *
 * This function will send the partial solution to a worker which has
 * completed his previously assigned work. As such this function must
 * also first receive the solution from the worker before sending out
 * the new work.
 *
 * @param solution      The valid solution. This is passed from within the
 *                      nqueens solver function.
 */
std::vector<unsigned int> solutions;
int num; 

void master_solution_func(std::vector<unsigned int>& solution) {
    // TODO: receive solutions or work-requests frnuom a worker and then
    //       proceed to send this partial solution to that worker.
    MPI_Status stat1, stat2;
    int s;
    MPI_Recv(&s, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat1); // recieve request
    int tag1 = stat1.MPI_TAG;
    int source = stat1.MPI_SOURCE;
    std::vector<unsigned int> done(s);
    if (tag1 == 1) { // if not first work request
        MPI_Recv(&done[0], s, MPI_UNSIGNED, source, MPI_ANY_TAG, MPI_COMM_WORLD, &stat2);
        for (int i = 0; i < s; i++) {
            solutions.push_back(done.at(i));
        }
    }
    done.clear();
    MPI_Send(&solution[0], num, MPI_UNSIGNED, source, 0, MPI_COMM_WORLD);
}



/**
 * @brief   Performs the master's main work.
 *
 * This function performs the master process' work. It will sets up the data
 * structure to save the solution and call the nqueens solver by passing
 * the master's callback function.
 * After all work has been dispatched, this function will send the termination
 * message to all worker processes, receive any remaining results, and then return.
 *
 * @param n     The size of the nqueens problem.
 * @param k     The number of levels the master process will solve before
 *              passing further work to a worker process.
 */
std::vector<unsigned int> master_main(unsigned int n, unsigned int k) {
    // TODO: send parameters (n,k) to workers via broadcast (MPI_Bcast)
    MPI_Bcast(&n, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    num = n;
    // allocate the vector for the solution permutations
    std::vector<unsigned int> pos(n);
    
    // generate all partial solutions (up to level k) and call the
    // master solution function
    nqueens_by_level(pos, 0, k, &master_solution_func);

    // TODO: get remaining solutions from workers and send termination messages
    int runningProcessors;
    MPI_Comm_size(MPI_COMM_WORLD, &runningProcessors);
    runningProcessors--;
    // send out termination messages
    while (runningProcessors > 0) { // run till all termination messages sent
        MPI_Status stat1, stat2, stat3;
        int s;
        MPI_Recv(&s, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &stat1);
        int source = stat1.MPI_SOURCE;
        int tag = stat1.MPI_TAG;
        std::vector<unsigned int> done(s);
        if (tag == 1) { // if this is not the first work request
            MPI_Recv(&done[0], s, MPI_UNSIGNED, source, MPI_ANY_TAG, MPI_COMM_WORLD, &stat2);
            for (int i = 0; i < s; i++) {
                solutions.push_back(done.at(i));
            }
        }
        int pointer = -1;
	    MPI_Send(&pointer, 1, MPI_INT, source, 2, MPI_COMM_WORLD);
        int d;
        MPI_Recv(&d, 1, MPI_INT, source, MPI_ANY_TAG, MPI_COMM_WORLD, &stat3);
        runningProcessors = runningProcessors - 1;
    }
    return solutions;
}

/**
 * @brief The workers' call back function for each found solution.
 *
 * This is the callback function for the worker processes, that is called
 * from within the nqueens solver, whenever a valid solution is found.
 *
 * This function saves the solution into the worker's solution cache.
 *
 * @param solution      The valid solution. This is passed from within the
 *                      nqueens solver function.
 */
void worker_solution_func(std::vector<unsigned int>& solution) {
    // TODO: save the solution into a local cache
    for (unsigned int i = 0; i < solution.size(); i++) {
        solutions.push_back(solution.at(i));
    }
}

/**
 * @brief   Performs the worker's main work.
 *
 * This function implements the functionality of the worker process.
 * The worker will receive partially completed work items from the
 * master process and will then complete the assigned work and send
 * back the results. Then again the worker will receive more work from the
 * master process.
 * If no more work is available (termination message is received instead of
 * new work), then this function will return.
 */
void worker_main() {
    unsigned int n, k;
    // TODO receive the parameters `n` and `k` from the master process via MPI_Bcast
    MPI_Bcast(&n, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);
    num = n;

    // TODO: implement the worker's functions: receive partially completed solutions,
    //       calculate all possible solutions starting with these queen positions
    //       and send solutions to the master process. then ask for more work.
    bool first = true;
    while (true) {
        if (first) { // if just started, send just one message
            int a = 0;
            MPI_Send(&a, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            first = false;
        } else { // send length of solultion vector and solution
            int s = solutions.size();
            MPI_Send(&s, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(&solutions[0], s, MPI_UNSIGNED, 0, 1, MPI_COMM_WORLD);
            solutions.clear();
        }
        MPI_Status stat; // start to recieve partial solution
        std::vector<unsigned int> solution(n);
        MPI_Recv(&solution[0], n, MPI_UNSIGNED, 0, MPI_ANY_TAG, MPI_COMM_WORLD,&stat); 
        int tag = stat.MPI_TAG;
        if (tag == 2) { // termination message
            int d = 1;
            MPI_Send(&d, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            return;
        }
        nqueens_by_level(solution, k, n, &worker_solution_func);
    }
}
