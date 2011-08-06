// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-7-12 Zou xu <zouivex@gmail.com>
//
#include <functional>
#include <queue>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <rime/impl/prism.h>
#include <rime/impl/syllablizer.h>

namespace rime {

typedef std::pair<int, SpellingType> Vertex;
typedef std::priority_queue<Vertex, std::vector<Vertex>, std::greater<Vertex> > VertexQueue;

int Syllablizer::BuildSyllableGraph(const std::string &input, Prism &prism, SyllableGraph *graph) {
  if (input.empty())
    return 0;

  int farthest_reach = 0;
  VertexQueue queue;
  queue.push(Vertex(0, kNormalSpelling));  // start

  while (!queue.empty()) {
    Vertex vertex(queue.top());
    queue.pop();
    int current_pos = vertex.first;

    // record a visit to the vertex
    VertexMap::iterator it = graph->vertices.find(current_pos);
    if (it == graph->vertices.end())
      graph->vertices.insert(vertex);
    else if (vertex.second < it->second)  // favor normal spelling
      it->second = vertex.second;

    // see where can we go by advancing a syllable
    std::vector<Prism::Match> matches;
    prism.CommonPrefixSearch(input.substr(current_pos), &matches);
    if (!matches.empty()) {
      EndVertexMap &end_vertices(graph->edges[current_pos]);
      BOOST_FOREACH(const Prism::Match &m, matches) {
        if (m.length == 0) continue;
        int end_pos = current_pos + m.length;
        if (end_pos > farthest_reach)
          farthest_reach = end_pos;
        SpellingMap &spellings(end_vertices[end_pos]);
        // TODO:
        // since SpellingAlgebra is not available yet,
        // we assume spelling resembles exactly the syllable itself.
        SyllableId syllable_id = m.value;
        // add a syllable with default properties to the edge's spelling-to-syllable map
        spellings.insert(SpellingMap::value_type(syllable_id, SpellingMappingProperty()));
        // again, we only have normal spellings for now
        queue.push(Vertex(end_pos, kNormalSpelling));
      }
    }
  }

  // TODO:
  
  graph->input_length = input.length();
  graph->interpreted_length = farthest_reach;
  
  return farthest_reach;
}

}  // namespace rime
