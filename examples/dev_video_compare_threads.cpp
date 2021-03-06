
#include <iostream>
#include <fstream>
#include <string>
#include <sstream> 

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h> 
#include <algorithm>
#include <vector>
#include <thread>
#include "pHash.cpp"

using namespace std;


void free_mem(ulong64 **hashes, ulong64 **hashes2, int *lengths, int *lengths2 ){
    free(hashes);
    free(hashes2);
    free(lengths);
    free(lengths2);
    
}

int CountElemInDir(const char *dir_name) {
    struct dirent *dir_entry;
    int dir_count = 0;
    
    //Openning Dir
    DIR *dir = opendir(dir_name);
    
    //Verifing that the directory is correctly open
    if (!dir) {
        printf("unable to open directory: %s \n", dir_name);
        exit(1);
    }
    
    //count directory elements
    while ((dir_entry = readdir(dir)) != 0) {
        if (strcmp(dir_entry->d_name, ".") && strcmp(dir_entry->d_name, "..")){
            dir_count++;
        }
    }
    
    //Close Dir
    closedir(dir);
    
    return dir_count;
}

vector<string> GetListOfPath(const char *dir_name) {
    vector<string> paths;
    struct dirent *dir_entry;
    char path[100];
    path[0] = '\0';
    
    //Openning Dir
    DIR *dir = opendir(dir_name);
    
    while ((dir_entry = readdir(dir)) != 0) {
        path[0] = '\0';
        
        if (strcmp(dir_entry->d_name, ".") && strcmp(dir_entry->d_name, "..")) {
            strcat(path, dir_name);
            strcat(path, dir_entry->d_name);
            //printf("Saving file path %s\n", path);
           	paths.push_back(path);
        }
    }
    
    //Close Dir
    closedir(dir);
    
    return paths;
}

vector<string> slice(const vector<string>& v, int start=0, int end=-1) {
    int oldlen = v.size();
    int newlen;
    
    if (end == -1 or end >= oldlen){
        newlen = oldlen-start;
    } else {
        newlen = end-start;
    }
    
    vector<string> nv(newlen);
    
    for (int i=0; i<newlen; i++) {
        nv[i] = v[start+i];
    }
    return nv;
}

vector<string> split_string(string& str, char delim = ' ')
{
    vector<string> split;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        split.push_back(token);
    }

    return split;
}

void task(vector<string> paths, int *lengths, ulong64 **hashes) {
	char path[100];
	path[0] = '\0';

    for(std::vector<string>::size_type i = 0; i != paths.size(); i++) {
    	path[0] = '\0';
    	strcpy(path, paths[i].c_str());
    	printf("opening file %s\n", path);

        // calculate the hash
    	hashes[i] = ph_dct_videohash(path, lengths[i]);
        if (hashes[i] == NULL){
            printf("this hash is NULL \n");
        }
    }
}

int main(int argc, char **argv) {
	int nb_threads = 1;

	if (argc == 4)
    {
    	nb_threads = atoi(argv[3]);
    } else if (argc != 3){
        printf("invalid number of arguments, please provide two paths to separate folders containing only videos\n");
        exit(1);
    }

    printf("Number of threads : %d\n", nb_threads);

    // getting directory names
    const char *dir_name = argv[1];
    int dir1_count = 0;
    const char *dir_name2 = argv[2];
    int dir2_count = 0;
    
    //Counting number of file in dir1
    dir1_count = CountElemInDir(dir_name);
    dir2_count = CountElemInDir(dir_name2);

    /*
    printf("Number of Files in dir1 %d\n", dir1_count);
    printf("Number of Files in dir2 %d\n", dir2_count);
    */
    
    //use number of files counted above to alloate proper memory
    // DIR1
    ulong64 **hashes = (ulong64 **)malloc(sizeof(ulong64 *) * (dir1_count));
    int *lengths = (int *)malloc(sizeof(int) * (dir1_count));

    // DIR2
    ulong64 **hashes2 = (ulong64 **)malloc(sizeof(ulong64 *) * (dir2_count));
    int *lengths2 = (int *)malloc(sizeof(int) * (dir2_count));
    
    //getting the list of paths
    vector<string> paths = GetListOfPath(dir_name);
    //extract filenames from paths for labeling csv file
    vector<string> files;
    for (auto& path: paths){
        vector<string> temp = split_string(path, '/');
        if(temp.size() > 0){
            files.push_back(temp[temp.size()-1]);
        }else{
            printf("could not split out filename\n");
        }
    } 

    //also getting file names for second dir
    vector<string> paths2 = GetListOfPath(dir_name2);
    vector<string> files2;
    for (auto& path: paths2){
        vector<string> temp = split_string(path, '/');
        if(temp.size() > 0){
            files2.push_back(temp[temp.size()-1]);
        }else{
            printf("could not split out filename\n");
        }
    }
    /*
    //printing path Recived
    printf("Paths of videos to Hash:\n");
    for (int i = 0; i < paths.size(); i++) 
        cout << paths[i] << "\n";
    for (int i = 0; i < paths2.size(); i++) 
        cout << paths2[i] << "\n";
    */

    //Get hashes
    errno = 0;
    std::vector<thread> threads(nb_threads);
    int nb_path_per_thread = paths.size()/(nb_threads-1);
    int position_in_vector = 0;
    vector<string> thread_paths;
    for (int i = 0; i < nb_threads; ++i)
    {
        if(i == nb_threads-1){
            //printf("Thread %d slice : %d à %d \n", i, position_in_vector, (int)(paths.size()-1) );
    	    thread_paths = slice(paths, position_in_vector, paths.size() );
        }else{
            //printf("Thread %d slice : %d à %d \n", i, position_in_vector, (position_in_vector+nb_path_per_thread));
    	    thread_paths = slice(paths, position_in_vector, position_in_vector+nb_path_per_thread);
        }
    	
    	/*
    	for (int i = 0; i < thread_paths.size(); i++) 
        	cout << thread_paths[i] << "\n";
        */

    	threads[i] = thread(task, thread_paths, &lengths[position_in_vector], &hashes[position_in_vector]);

        position_in_vector = position_in_vector + nb_path_per_thread;
    }

    for (auto& th : threads) {
        th.join();
    }

    if (errno) {
        printf("error reading directory\n");
        free_mem(hashes,hashes2,lengths,lengths2);
        exit(1);
    }


    errno = 0;
    std::vector<thread> threads2(nb_threads);
    nb_path_per_thread = paths2.size()/(nb_threads-1);
    position_in_vector = 0;
    for (int i = 0; i < nb_threads; ++i)
    {
        if( i == nb_threads-1){
            //printf("Thread %d slice : %d à %d \n", i, position_in_vector, (int)(paths2.size()-1) );
    	    thread_paths = slice(paths2, position_in_vector, paths2.size());
        }else{
            //printf("Thread %d slice : %d à %d \n", i, position_in_vector, (position_in_vector+nb_path_per_thread));
            thread_paths = slice(paths2, position_in_vector, position_in_vector+nb_path_per_thread);
        }

    	
    	/*
    	for (int i = 0; i < thread_paths.size(); i++) 
        	cout << thread_paths[i] << "\n";
        */

        threads2[i] = thread(task, thread_paths, &lengths2[position_in_vector], &hashes2[position_in_vector]);

        position_in_vector = position_in_vector + nb_path_per_thread;
    }

    for (auto& th : threads2) {
        th.join();
    }

    if (errno) {
        printf("error reading directory\n");
        free_mem(hashes,hashes2,lengths,lengths2);
        exit(1);
    }

    //now we have the hashes and can move to calculating distance
    
    
    int** dist = new int*[dir2_count];
    for(int i = 0; i < dir2_count; ++i)
        dist[i] = new int[dir1_count];

    printf("calculating video distances\n");

    for(int i =0; i < dir1_count; i++){
        //for each original file, compare with all other video hashes
        for(int j = 0; j < dir2_count; j++){
            //printf("dist btween vid: %d, and vid: %d\n", i,j);
            dist[i][j] = ph_hamming_distance(*hashes[i], *hashes2[j]);
        }
    }


    std::ofstream outfile("hash.csv");
    /*
    for(int i=-1; i< dir1_count; i++){
        for(int j =0; j < dir2_count; j++){
            //print headers
            if (i == -1){
                outfile << files[j];
            }else{
                outfile << dist[i][j];
            }

            if(j < dir2_count-1){
                outfile << ',';
            }
        }
        outfile << endl;
    }

    */

    for(int i = 0; i < dir1_count; i++){
        outfile << files[i] << ',' << *hashes[i] << endl;
    }
    for(int i = 0; i < dir2_count; i++){
        outfile << files2[i] << ',' << *hashes2[i] << endl;
    }
    
    //free up our distance array
    delete(dist);
    outfile.close();



    // free up allocated memory
    free_mem(hashes,hashes2,lengths,lengths2);

    return 0;
}
