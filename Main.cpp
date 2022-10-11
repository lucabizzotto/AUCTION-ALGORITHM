#include "Header.h"

int main()
{
    my_graph g(0);
    
    add_edge(0, 6, EdgeProperty(9), g);
    add_edge(0, 7, EdgeProperty(6), g);
    add_edge(0, 9, EdgeProperty(3), g);
    add_edge(0, 11, EdgeProperty(2), g);
    add_edge(1, 7, EdgeProperty(2), g);
    add_edge(1, 8, EdgeProperty(7), g);
    add_edge(1, 10, EdgeProperty(1), g);
    add_edge(2, 6, EdgeProperty(5), g);
    add_edge(2, 7, EdgeProperty(4), g);
    add_edge(2, 11, EdgeProperty(3), g);
    add_edge(3, 7, EdgeProperty(6), g);
    add_edge(3, 8, EdgeProperty(8), g);
    add_edge(3, 9, EdgeProperty(3), g);
    add_edge(3, 10, EdgeProperty(4), g);
    add_edge(4, 6, EdgeProperty(8), g);
    add_edge(4, 8, EdgeProperty(4), g);
    add_edge(4, 10, EdgeProperty(1), g);
    add_edge(5, 9, EdgeProperty(7), g);
    add_edge(5, 10, EdgeProperty(6), g);
    add_edge(5, 11, EdgeProperty(5), g);

    std::cout << "the given graph G" << std::endl;
    write_graphviz(std::cout, g);
    Support s(g);
    s.matching();
}