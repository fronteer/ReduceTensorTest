#ifndef _APP_ARGS_HPP_
#define _APP_ARGS_HPP_

#include <iostream>
#include <stdexcept>
#include <vector>

#include <getopt.h>

static std::vector<int> get_int_sequence(std::string seqStr)
{
    std::vector<int> lengths;
    std::size_t pos = 0;
    std::size_t new_pos;

    new_pos = seqStr.find(',', pos);
    while(new_pos != std::string::npos)
    {
        std::string sliceStr = seqStr.substr(pos, new_pos - pos);

        int len = std::stoi(sliceStr);

        lengths.push_back(len);

        pos     = new_pos + 1;
        new_pos = seqStr.find(',', pos);
    };

    std::string sliceStr = seqStr.substr(pos);
    int len              = std::stoi(sliceStr);

    lengths.push_back(len);

    return (lengths);
}

static const struct option long_options[] = {
                   {"dimLengths", required_argument, NULL, 'D'},
                   {"reduceDims", required_argument, NULL, 'R'},
                   {"reduceOp", required_argument, NULL, 'O'},
                   {"nanPropa", no_argument, NULL, 'N'},
                   {"indices", no_argument, NULL, 'I'},
                   {"help", no_argument, NULL, 'H'},
                   {"verify", no_argument, NULL, 'V'}, 
                   {0, 0, 0, 0}       };

class AppArgs 
{
private: 
    int option_index;

    void show_usage()
    {
       std::cout << "Arguments: " << std::endl << std::endl; 

       std::cout << "--dimLengths <xxx>  ---the lengths of the input tensor dimensions" << std::endl << std::endl; 
       std::cout << "--reduceDims <xxx>  ---the indexes of the dimensions to reduce" << std::endl << std::endl; 
       std::cout << "--reduceOp <xxx> ---the id of the reduce operation (0 for add, 1 for mul, 2 for min, ...)" << std::endl << std::endl; 
       std::cout << "--nanPropa ---enable nanPropagation" << std::endl << std::endl; 
       std::cout << "--indices ---enable the reduce indices" << std::endl << std::endl; 
       std::cout << "--verify ---verify the device computed result by comparing to the host computed result" << std::endl << std::endl; 
       std::cout << "--help ---show the above information" << std::endl << std::endl; 
    }; 

public: 
    AppArgs() = default; 

    int processArgs(int argc, char *argv[])
    {
        unsigned int ch;

        while (1) {
           ch = getopt_long (argc, argv, "", long_options, &option_index);
           if ( static_cast<int>(ch) == -1 )
                break;
           switch (ch) {
               case 'D':
                   inLengths = get_int_sequence(optarg);
                   break;
               case 'R':
                   toReduceDims = get_int_sequence(optarg);
                   break;
               case 'O':
#ifdef CUDA_CUDNN_APP		   
                   op = static_cast<cudnnReduceTensorOp_t>(op);
#else
                   op = static_cast<miopenReduceTensorOp_t>(op);
#endif		   
                   break;
               case 'N':
#ifdef CUDA_CUDNN_APP		   
                   nanPropaOpt = CUDNN_PROPAGATE_NAN;
#else
                   nanPropaOpt = MIOPEN_PROPAGATE_NAN;
#endif		   
                   break;
               case 'I':
#ifdef CUDA_CUDNN_APP		   
                   indicesOpt = CUDNN_REDUCE_TENSOR_FLATTENED_INDICES;
#else
                   indicesOpt = MIOPEN_REDUCE_TENSOR_FLATTENED_INDICES;
#endif 
                   break;
               case 'V':
	           doVerify = true; 
	           break; 	   
               case 'H':
                   show_usage();
                   return(-1);
               default:
                   std::cerr << "Wrong arguments format!" << std::endl;
                   show_usage();
                   throw std::runtime_error("Wrong command-line format!");
           }
        }

        if ( inLengths.empty() || toReduceDims.empty() || toReduceDims.size() > inLengths.size() ) 
             throw std::runtime_error("Wrong command-line format/parameters!");

        for(const auto & dimIndex : toReduceDims) 
            if ( dimIndex >= inLengths.size() ) 
                 throw std::runtime_error("Wrong command-line parameters(toReduceDims not correct)!");


        outLengths = inLengths;

        // set the lengths of the dimensions to be reduced to 1 to represent the output Tensor
        for(int i  = 0; i < toReduceDims.size(); i++)
             outLengths[toReduceDims[i]] = 1;

        inStrides.resize(inLengths.size());
        inStrides[inStrides.size()-1] = 1;
        for(int i=inStrides.size()-2; i >= 0; i--)
            inStrides[i] = inStrides[i+1] * inLengths[i+1];

        outStrides.resize(outLengths.size());
        outStrides[outStrides.size()-1] = 1;
        for(int i=inStrides.size()-2; i >= 0; i--)
            outStrides[i] = outStrides[i+1] * outLengths[i+1];

        for(int i = 0; i < inLengths.size(); i++)
            if(inLengths[i] == outLengths[i])
               invariantDims.push_back(i);


        return(0); 
    }; 

protected: 
#ifdef CUDA_CUDNN_APP
    cudnnReduceTensorOp_t op = CUDNN_REDUCE_TENSOR_ADD;
    cudnnDataType_t dataType = CUDNN_DATA_FLOAT;
    cudnnDataType_t compType = CUDNN_DATA_FLOAT;
    cudnnNanPropagation_t nanPropaOpt =  CUDNN_NOT_PROPAGATE_NAN;
    cudnnReduceTensorIndices_t indicesOpt = CUDNN_REDUCE_TENSOR_NO_INDICES;
    cudnnIndicesType_t indicesType = CUDNN_32BIT_INDICES;
#else
    miopenReduceTensorOp_t op = MIOPEN_REDUCE_TENSOR_ADD;
    miopenDataType_t dataType = miopenFloat;
    miopenDataType_t compType = miopenFloat;
    miopenNanPropagation_t nanPropaOpt =  MIOPEN_NOT_PROPAGATE_NAN;
    miopenReduceTensorIndices_t indicesOpt = MIOPEN_REDUCE_TENSOR_NO_INDICES;
    miopenIndicesType_t indicesType = MIOPEN_32BIT_INDICES;
#endif
    std::vector<int> inLengths;
    std::vector<int> inStrides;

    std::vector<int> outLengths;
    std::vector<int> outStrides;

    std::vector<int> invariantDims;
    std::vector<int> toReduceDims;

    bool doVerify = false; 
}; 


#endif

