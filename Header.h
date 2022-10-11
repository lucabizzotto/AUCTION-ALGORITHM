#include <iostream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/one_bit_color_map.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/bipartite.hpp>
#include <boost/graph/graphviz.hpp>

using namespace boost;

typedef property< edge_weight_t, float> EdgeProperty;
typedef adjacency_list< vecS, vecS, undirectedS, no_property, EdgeProperty> my_graph;
typedef std::vector <default_color_type> partition_t;
typedef vec_adj_list_vertex_id_map <no_property, unsigned int> index_map_t;
typedef iterator_property_map <partition_t::iterator, index_map_t> partition_map_t;

typedef std::pair<graph_traits<my_graph>::vertex_descriptor, graph_traits<my_graph>::vertex_descriptor> pair_matching;
typedef std::vector<pair_matching> pair_matching_vector;
typename graph_traits <my_graph>::vertex_iterator vertex_iter, vertex_end;
typename graph_traits <my_graph>::out_edge_iterator out_i, out_end;
typename property_map<my_graph, edge_weight_t>::type weightmap;
typename graph_traits<my_graph>::edge_descriptor edge_descriptor;


class Support
{
public:
     //constructor 
     Support(my_graph& g_) : g(g_){};


     //main function
     void matching()
     {
        auction_algorithm();
        create_matrix();
        do_auction();
        populate_best_matching();
        summary_information();
     }


     /*
     * return the weighted matching
     */
     pair_matching_vector get_matching()
     {
         return matching_vector;
     }


     
     /*
     * populate the vector of matching
     */
     void populate_best_matching()
     {
         for (auto iter : object_map) 
         {
             matching_vector.push_back(pair_matching(iter.first, iter.second.owner));
         }
     }


    /*
    * print the matrix that store the information regarding the graph
    * @param matrix take the matrix to print
    */
    void print_matrix()
    {
        std::cout << "matrix : " << std::endl;
        for (size_t i = 0; i < matrix.size(); i++)
        {
            for (size_t j = 0; j < matrix[i].size(); j++)
            {
                std::cout << matrix[i][j] << "    ";
            }
            std::cout << std::endl;
        }
    }


    /*
    * initialize the matrix
    * @param matrix  initilize it with the right dimension
    */
    void initialize_matrix(std::vector<std::vector<float>>& matrix)
    {
        matrix.resize(buyers.size());
        for (size_t i = 0; i < buyers.size(); i++)
        {
            matrix[i].resize(objects.size());
        }
    }


    /*
    * find the best and second best object for buyer_i
    * @param buyer_i indicate the buyer_i
    * @return a vector containing the best_object in position 0 and the second best object in position 1
    */
    std::vector <graph_traits<my_graph>::vertex_descriptor> find_best_object(graph_traits<my_graph>::vertex_descriptor buyer_i)
    {
        //position of buyer i in the matrix
        int i = buyer_map[buyer_i];
        graph_traits<my_graph>::vertex_descriptor bestObj;
        graph_traits<my_graph>::vertex_descriptor snd_bestObj;
        //search of maximun

        auto iter = object_map.begin();
        float max = matrix[i][0] - object_map[iter->first].price_object;
       
        auto first_object = iter->first;

        iter++; // increment iterator

        float snd_max = matrix[i][1] - object_map[iter->first].price_object;
        auto snd_object = iter->first;
        
        iter++; // increment iterator( third position)
        //save the position
        
        if (max < snd_max)
        {
            std::swap(max, snd_max);
            std::swap(first_object, snd_object);
        }

        for (size_t j = 2; j < matrix[0].size(); j++)
        {
            if (matrix[i][j] - object_map[iter->first].price_object > max)
            {
                snd_max = max;
                max = matrix[i][j] - object_map[iter->first].price_object;
                snd_object = first_object;
                first_object = iter->first;//i save the position of the best object 
            }
            else
            {
                if (matrix[i][j] - object_map[iter->first].price_object > snd_max)
                {
                    snd_max = matrix[i][j] - object_map[iter->first].price_object;
                    snd_object = iter->first;
                }
            }
            iter++; // increment the iterator
        }
        bestObj = first_object;
        snd_bestObj = snd_object;
     
        std::vector<graph_traits<my_graph>::vertex_descriptor> res;
        res.push_back(bestObj);
        res.push_back(snd_bestObj);
        return res;
    }


    /*
    * find the right bid to do
    * @param best_objects indicate the two best object for buyer_i
    */
    void bid(graph_traits<my_graph>::vertex_descriptor buyer_i, std::vector <graph_traits<my_graph>::vertex_descriptor>& best_objects)
    {
        int i = object_map[best_objects[0]].index;
        int j = object_map[best_objects[1]].index;
        int buyer = buyer_map[buyer_i];
        
        float offer = (matrix[buyer][i] - object_map[best_objects[0]].price_object) - (matrix[buyer][j] - object_map[best_objects[1]].price_object);
        if (offer >= 0.1)
        {
            object_map[best_objects[0]].price_object += offer;//increase the price 
            
            if (object_map[best_objects[0]].check_owner)//there is alredy an owner 
            {
                buyers.push_back(object_map[best_objects[0]].owner);//insert the buyer the we make free again in the list of buyer 
            }

            object_map[best_objects[0]].owner =  buyer_i;//associate the object to the buyer 
            object_map[best_objects[0]].check_owner = true;  // set true the owner
            std::remove(buyers.begin(), buyers.end(), buyer_i);//remove buyer_i from the list
            buyers.resize(buyers.size() - 1);//i have to do since remove doesn't change the size 
           
        }
        else//not profitable object for buyer_i so i remove from the list of unsigned buyer 
        {
            std::remove(buyers.begin(), buyers.end(), buyer_i);//remove buyer_i from the list
            buyers.resize(buyers.size() - 1);//i have to do since remove doesn't change the size 
        }

    }


    /*
    * I create a matrix to store information about the graph. Each Aij rappresent the weight of the edge (i,j)  E
    * and initiliaze the map :
    * buyer_map : indicate the index of buyer i in the matrix
    * pos_matrix_to_buyer: indicate which is the buyer that is in position i
    * object_map: indicate the index  of object i in the matrix
    *  pos_matrix_to_object : indicate which is the object that is in position i
    *  price_object_map : indicate the price of the object
    */
    void  create_matrix()
    {
        //i need the next two for to map the object and buyer to the corrisponding index in the matrix 
        int counter = 0;//incremental position on the matrix 
        std::cout << "partition : " << std::endl;
        std::cout << "buyers:" << std::endl;
        //buyer
        for (auto iter : buyers)
        {
            std::cout << iter << "\t";
            
            buyer_map.insert_or_assign(iter, counter++);
        }
        std::cout << "\nobject:" << std::endl;
        //object
        counter = 0;
        for (auto j : objects)
        {
            std::cout << j << "\t";
            // create the object and intialization
            object_map.insert_or_assign(j, Info(counter++));
        }
        std::cout << std::endl;

        weightmap = get(edge_weight, g);
        initialize_matrix(matrix);
        std::cout << std::endl;

        for (auto i : buyers)
        {
            for (tie(out_i, out_end) = out_edges(i, g);
                out_i != out_end; ++out_i) {
                //The iterators are called out-edge iterators and dereferencing one of these iterators gives an edge descriptor object
                edge_descriptor = *out_i;
                matrix[buyer_map[edge_descriptor.m_source]][object_map[edge_descriptor.m_target].index] = get(weightmap, *out_i);
            }
        }
        print_matrix();
    }


    /*
    * It does the auction, for each buyer find the best objects and then call the bid to do the offer
    */
    void do_auction()
    {
        while (buyers.size() > 0)
        {
            std::vector <graph_traits<my_graph>::vertex_descriptor> best1 = find_best_object(buyers[0]);
            bid(buyers[0], best1);
        }
    }


    /*
    * it print the summary information of the auction algorithm
    */
    void summary_information()
    {
        std::cout << std::endl;
        std::cout << "price of each object : " << std::endl;
        for (auto iter : object_map)
        {
            std::cout << "object : " << iter.first << ", price : " << iter.second.price_object << std::endl;
        }
        std::cout << std::endl;
        std::cout << "owner for each object :" << std::endl;
        for (auto iter : object_map)
        {
            std::cout << "object " << iter.first << " own by the buyer " << iter.second.owner << std::endl;
        }
    }


    /*
    * divide the nodes in buyer and objects using the partition that i found it
    * @map map has the partition color white v1 color black v2
    */
    template<typename  iterator_property_map>
    void auction_algorithm_with_partition(iterator_property_map map)
    {
        for (boost::tie(vertex_iter, vertex_end) = vertices(g); vertex_iter != vertex_end; ++vertex_iter)
        {
            
            if (get(map, *vertex_iter) == color_traits <default_color_type>::white())
            {
                buyers.push_back(*vertex_iter);
            }
            else
            {
                objects.push_back(*vertex_iter);
            }
        }
    }


    /*
    * checks if the graph is bipartite and then call is_bipartite and auction_algorithm_with_partition
    */
    void auction_algorithm()
    {
        bool bipartite = is_bipartite(g);
        //check if the graph is bipartite
        if (bipartite)
        {
            partition_t partition(num_vertices(g));
            partition_map_t partition_map(partition.begin(), get(vertex_index, g));
            /**
          * Checks a given graph for bipartiteness and fills the given color map with
          * white and black according to the bipartition. If the graph is not
          * bipartite, the contents of the color map are undefined. Runs in linear
          * time in the size of the graph, if access to the property maps is in
          * constant time.
          *
          * @param graph The given graph.
          * @param index_map An index map associating vertices with an index.
          * @param partition_map A color map to fill with the bipartition.
          * @return true if and only if the given graph is bipartite.
          */
            is_bipartite(g, get(vertex_index, g), partition_map);
            auction_algorithm_with_partition(partition_map);
        }
        else
        {
            std::cerr << "The graph is not bipartite:";
        }
    }



private:
    /*
    * Inner class that i used as a container to store information about the object 
    * class member :
    *   -index : maintain the reference position in the matrix and the object 
    *   -price_object : contain the price of the object initialy is set to 0
    *   -check_owner : it is a flag and its aim it is to know if the object as an owner or not 
    *   -owner : the owner of the object 
    */
    class Info
    {
    public:
        Info() {};

        Info(int index_) :
            index(index_), price_object(0), check_owner(false) {};

        int index;

        float price_object;

        bool check_owner;  // flag to check if there is an owner

        graph_traits<my_graph>::vertex_descriptor owner;

    };

    //the given graph
    my_graph& g;

    //vector contains the matching output
    pair_matching_vector matching_vector;

    //matrix that store information regarding the graph
     //each Aij rappresent the weight of the edge i,j of the graph 
     //where i is the buyer i and j the object j
    std::vector<std::vector<float>> matrix;

    //map to save the position of the buyer in the matrix 
    std::map< graph_traits<my_graph>::vertex_descriptor, int> buyer_map;

    //map that store the information regarding the object 
    std::map< graph_traits<my_graph>::vertex_descriptor, Info> object_map;

    //vector with the buyer
    std::vector< graph_traits<my_graph>::vertex_descriptor> buyers;

    //vector with the object
    std::vector< graph_traits<my_graph>::vertex_descriptor> objects;
};
