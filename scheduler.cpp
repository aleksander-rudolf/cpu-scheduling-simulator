#include "scheduler.h"
#include "common.h"
#include "iostream"

// runs Round-Robin scheduling simulator
// input:
//   quantum = time slice
//   max_seq_len = maximum length of the reported executing sequence
//   processes[] = list of process with populated IDs, arrival_times, and bursts
// output:
//   seq[] - will contain the execution sequence but trimmed to max_seq_len size
//         - idle CPU will be denoted by -1
//         - other entries will be from processes[].id
//         - sequence will be compressed, i.e. no repeated consecutive numbers
//   processes[]
//         - adjust finish_time and start_time for each process
//         - do not adjust other fields
//
void simulate_rr(int64_t quantum, int64_t max_seq_len, std::vector<Process> & processes, std::vector<int> & seq) {

    seq.clear();
    int64_t curr_time = 0;
    std::vector<int> rq, jq;
    std::vector<int64_t> remaining_bursts;

    for(auto &process : processes){
        jq.push_back(process.id);
        remaining_bursts.push_back(process.burst);
    }

    while(true){

        //All processes completed and there are no additional processes to start, end the simulation.
        if(rq.empty() && jq.empty()){
            break;
        }

        //No processes in progress and there are additional processes to start.
        if(rq.empty() && !jq.empty()){
            rq.push_back(jq.at(0));
            jq.erase(jq.begin());
            curr_time = processes.at(rq.at(0)).arrival_time;
            if((int64_t)seq.size() < max_seq_len && curr_time == 0){
                seq.push_back(rq.at(0));
            }
            else{
                seq.push_back(-1);
            }
        }

        //Processes in progress and there are additional processes to start.
        if(!rq.empty() && !jq.empty()){
            //Process arriving at curr_time
            if(processes.at(jq.at(0)).arrival_time == curr_time){
                rq.push_back(jq.at(0));
                jq.erase(jq.begin());
                continue;
            }

            //Determine how many quantums can be safely skipped for every process in progress
            //if they all have more than one quantum unit of time in remaining_bursts,
            //before next process from the job queue arrives, and before any of the current
            //processes in progress finish.
            bool flag = true;
            int64_t min_bursts = remaining_bursts.at(rq.at(0));
            for(int i = 0; i < (int)rq.size(); i++){
                if(remaining_bursts.at(rq.at(i)) <= quantum){
                    flag = false;
                    break;
                }
                if(remaining_bursts.at(rq.at(i)) < min_bursts){
                    min_bursts = remaining_bursts.at(rq.at(i));
                }
            }

            int64_t n = min_bursts/quantum;
            if(min_bursts%quantum == 0){
                n--;
            }
            int64_t m = (processes.at(jq.at(0)).arrival_time - curr_time)/((int)rq.size()*quantum);
            if((curr_time + (int)rq.size()*quantum) < processes.at(jq.at(0)).arrival_time){
                if(flag){
                    int64_t k = std::min(n, m);
                    for(int i = 0; i < (int)rq.size(); i++){
                        if(processes.at(rq.at(i)).start_time == -1){
                            processes.at(rq.at(i)).start_time = curr_time+quantum*i;
                        }
                        remaining_bursts.at(rq.at(i)) -= quantum*k;
                    }
                    curr_time += (int)rq.size()*quantum*k;
                    for(int64_t i = 0; i < k && i < max_seq_len; i++){
                        for(int j = 0; j < (int)rq.size(); j++){
                            if((int64_t)seq.size() < max_seq_len && seq.back() != rq.at(j)){
                                seq.push_back(rq.at(j));
                            }
                        }
                    }
                }
            }

            //Process arrived from the job-queue while all processes in progress
            //had more than one quantum time unit left to completion or there is
            //a process(es) in the ready-queue that has less than one quantum unit of
            //time, incremeant curr_time by one quantum at a time.
            if(remaining_bursts.at(rq.at(0)) > quantum){
                if(processes.at(rq.at(0)).start_time == -1){
                    processes.at(rq.at(0)).start_time = curr_time;
                }
                curr_time += quantum;
                if((int64_t)seq.size() < max_seq_len && seq.back() != rq.at(0)){
                    seq.push_back(rq.at(0));
                }
                remaining_bursts.at(rq.at(0)) -= quantum;
                
                while(!jq.empty() && processes.at(jq.at(0)).arrival_time < curr_time){
                    rq.push_back(jq.at(0));
                    jq.erase(jq.begin());
                }
                
                if(!jq.empty() && processes.at(jq.at(0)).arrival_time == curr_time){
                    rq.push_back(rq.at(0));
                    rq.erase(rq.begin());
                    rq.push_back(jq.at(0));
                    jq.erase(jq.begin());
                }
                else{
                    rq.push_back(rq.at(0));
                    rq.erase(rq.begin());
                }
                continue;
            }

            //Process currently in progress has less than or equal to one quantum time unit left to completion,
            //incremeant curr_time by the remaining_burst of the process in progress.
            if(remaining_bursts.at(rq.at(0)) <= quantum){
                if(processes.at(rq.at(0)).start_time == -1){
                    processes.at(rq.at(0)).start_time = curr_time;
                }
                curr_time += remaining_bursts.at(rq.at(0));
                if((int64_t)seq.size() < max_seq_len && seq.back() != rq.at(0)){
                    seq.push_back(rq.at(0));
                }
                remaining_bursts.at(rq.at(0)) = 0;
                processes.at(rq.at(0)).finish_time = curr_time;
                rq.erase(rq.begin());
                if(processes.at(jq.at(0)).arrival_time <= curr_time){
                    rq.push_back(jq.at(0));
                    jq.erase(jq.begin());
                }
                continue;
            }
        }

        //Processes in progress and there are no additional processes to start.
        if(!rq.empty() && jq.empty()){

            //When only one job remains to be completed, finish the process, and end the simulation.
            if(rq.size() == 1){
                if(processes.at(rq.at(0)).start_time == -1){
                    processes.at(rq.at(0)).start_time = curr_time;
                }
                curr_time += remaining_bursts.at(rq.at(0));
                if((int64_t)seq.size() < max_seq_len && seq.back() != rq.at(0)){
                    seq.push_back(rq.at(0));
                }
                processes.at(rq.at(0)).finish_time = curr_time;
                remaining_bursts.at(rq.at(0)) = 0;
                rq.erase(rq.begin());
                break;
            }

            //Determine how many quantums can be safely skipped for every process in progress
            //if they all have more than one quantum unit of time in remaining_bursts,
            //before any of the current processes in progress finish.
            bool flag = true;
            int64_t min_bursts = remaining_bursts.at(rq.at(0));
            for(int i = 0; i < (int)rq.size(); i++){
                if(remaining_bursts.at(rq.at(i)) <= quantum){
                    flag = false;
                    break;
                }
                if(remaining_bursts.at(rq.at(i)) < min_bursts){
                    min_bursts = remaining_bursts.at(rq.at(i));
                }
            }

            int64_t n = min_bursts/quantum;
            
            if(min_bursts%quantum == 0){
                n--;
            }
            if(flag){
                for(int i = 0; i < (int)rq.size(); i++){
                    if(processes.at(rq.at(i)).start_time == -1){
                        processes.at(rq.at(i)).start_time = curr_time+quantum*i;
                    }
                    remaining_bursts.at(rq.at(i)) -= quantum*n;
                }
                curr_time += (int)rq.size()*quantum*n;
                for(int64_t i = 0; i < n && i < max_seq_len; i++){
                    for(int j = 0; j < (int)rq.size(); j++){
                        if((int64_t)seq.size() < max_seq_len && seq.back() != rq.at(j)){
                            seq.push_back(rq.at(j));
                        }
                    }
                }
            }

            //There is a process(es) in the ready-queue that has less than one quantum unit of
            //time, incremeant curr_time by one quantum at a time.
            if(remaining_bursts.at(rq.at(0)) > quantum){
                if(processes.at(rq.at(0)).start_time == -1){
                    processes.at(rq.at(0)).start_time = curr_time;
                }
                curr_time += quantum;
                if((int64_t)seq.size() < max_seq_len && seq.back() != rq.at(0)){
                    seq.push_back(rq.at(0));
                }
                remaining_bursts.at(rq.at(0)) -= quantum;
                rq.push_back(rq.at(0));
                rq.erase(rq.begin());
                continue;
            }

            //Process currently in progress has less than or equal to one quantum time unit left to completion,
            //incremeant curr_time by the remaining_burst of the process in progress.
            if(remaining_bursts.at(rq.at(0)) <= quantum){
                if(processes.at(rq.at(0)).start_time == -1){
                    processes.at(rq.at(0)).start_time = curr_time;
                }        
                curr_time += remaining_bursts.at(rq.at(0));
                if((int64_t)seq.size() < max_seq_len && seq.back() != rq.at(0)){
                    seq.push_back(rq.at(0));
                }
                remaining_bursts.at(rq.at(0)) = 0;
                processes.at(rq.at(0)).finish_time = curr_time;  
                rq.erase(rq.begin());
                continue;
            }       
        }     
    }
    return;
}