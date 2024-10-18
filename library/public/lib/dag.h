#pragma once
#ifndef LIB_DAG_H
#define LIB_DAG_H

#include "set.h"
#include "array.h"

namespace lib
{

/**
* Implementation of a directed acyclic graph (DAG).
* This implementation of a DAG is not thread-safe. Sorting is done single-threadedly.
*/
template <typename T, provides_memory provided_allocator = default_allocator>
class digraph
{
private:
	struct graph_edge
	{
		size_t from;	// Index of the source vertex.
		size_t to;		// Index of the destination vertex.
	};

	using value_type		= T;
	using const_reference	= value_type const&;
	using vertex_set		= set<T, provided_allocator>;
	using edge_list			= array<graph_edge, provided_allocator>;

	vertex_set	m_vertices;
	edge_list	m_edges;
	bool		m_is_acyclic;

	constexpr bool connection_exist(size_t from, size_t to) const
	{
		for (graph_edge const& edge : m_edges)
		{
			if (edge.from == from && 
				edge.to == to)
			{
				return true;
			}
		}
		return false;
	}

	constexpr void release()
	{
		m_vertices.release();
		m_edges.release();
	}

public:

	constexpr digraph() :
		m_vertices{}, m_edges{}, m_is_acyclic{ true }
	{}

	constexpr ~digraph() { release(); }

	constexpr digraph(size_t capacity) :
		digraph{}
	{
		m_vertices.reserve(capacity);
	}

	constexpr digraph(const digraph& rhs) :
		m_vertices{}, m_edges{}, m_is_acyclic{ true }
	{ 
		*this = rhs; 
	}

	constexpr digraph(digraph&& rhs) :
		m_vertices{}, m_edges{}, m_is_acyclic{ true } 
	{ 
		*this = std::move(rhs); 
	}

	constexpr digraph& operator=(const digraph& rhs)
	{
		if (this != &rhs)
		{
			m_vertices = rhs.m_vertices;
			m_edges = rhs.m_edges;
		}
		return *this;
	}

	constexpr digraph& operator=(digraph&& rhs)
	{
		if (this != &rhs)
		{
			m_vertices = std::move(rhs.m_vertices);
			m_edges = std::move(rhs.m_edges);

			new (&rhs) digraph{};
		}
		return *this;
	}

	/**
	* Adds a new unique vertex into the graph.
	*/
	template <typename... ForwardType>
	constexpr void add_vertex(ForwardType&&... args)
	{
		T key{ std::forward<ForwardType>(args)... };
		m_vertices.try_emplace(key, std::move(key));
	}

	/**
	* Adds a new unique vertex into the graph and creates an edge that symbolizes the relationship between those two vertices.
	*/
	template <typename... ForwardType>
	constexpr void add_edge(T&& from, ForwardType&&... args)
	{
		m_is_acyclic = false;
		T to{ std::forward<ForwardType>(args)... };

		if (from != to)
		{
			m_is_acyclic = true;

			auto result = m_vertices.try_insert(to, std::move(to));
			size_t bucket = result.value_or(vertex_set::invalid_bucket_v);

			if (bucket == Vertices_t::invalid_bucket())
			{
				bucket = m_vertices.bucket(to);
			}

			size_t parent = m_vertices.bucket(from);

			if (parent == vertex_set::invalid_bucket_v)
			{
				parent = m_vertices.insert(std::move(from));
			}

			if (!connection_exist(parent, bucket))
			{
				m_edges.emplace(parent, bucket);
			}
		}
	}

	constexpr bool relationship_exist(const T& from, const T& to)
	{
		size_t a = m_vertices.bucket(from);
		size_t b = m_vertices.bucket(to);

		return connection_exist(a, b);
	}

	constexpr bool is_acyclic() const
	{
		return m_is_acyclic;
	}

	// TODO(Afiq):
	// Introduce this in the future.
	/*bool remove_vertex(ConstRefType vertex)
	{
		size_t bucket = m_vertices.bucket(vertex);
		if (bucket == Vertices_t::invalid_bucket())
		{
			return false;
		}

		m_vertices.erase(vertex);

		for (size_t i = 0; i < m_edges.length();)
		{
			const Edge& edge = m_edges[i];
			if (edge.from == bucket ||
				edge.to == bucket)
			{
				m_edges.pop_at(i);
				continue;
			}
			++i;
		}

		return true;
	}*/

	constexpr size_t num_edges() const
	{ 
		return m_edges.size(); 
	}

	constexpr size_t num_vertices() const
	{ 
		return m_vertices.size(); 
	}

	constexpr size_t num_edges_for_vertex(ConstRefType key) const
	{
		size_t count = 0;
		const size_t bucket = m_vertices.bucket(key);
		if (bucket != vertex_set::invalid_bucket_v)
		{
			for (graph_edge const& edge : m_edges)
			{
				if (edge.from == bucket)
				{
					++count;
				}
			}
		}
		return count;
	}

	/**
	* Clears the content of the graph.
	*/
	constexpr void clear()
	{
		m_vertices.clear();
		m_edges.empty();
	}

	/**
	* Topological sort.
	* 
	* Returns an ascending ordered list of T based on it's dependencies.
	* If the graph is cyclic, the method returns an empty list.
	*/
	constexpr array<T, provided_allocator> sort()
	{
		array<T, provided_allocator> result{ m_vertices.size() };

		// Make copies of edges and vertices so that we don't modify the DAG's internal data.
		edge_list	edges = m_edges;
		vertex_set	vertices = m_vertices;

		auto loop_edges = [&](size_t& node, graph_edge* pEdge) -> bool 
		{
			// Check if the node being passed into this lambda contain any edges.
			for (graph_edge const& e : edges)
			{
				// If the current node contains an edge, we set the current node to the vertex at the other end of the edge.
				// The edge is then cached in "pEdge". Doing this ensures that we can traverse back to the previous node.
				// When this lambda returns true, the algorithm will continue traversing down the graph until a child it finds a node with no edges.
				if (e.from == node)
				{
					node = e.to;
					*pEdge = e;
					return true;
				}
			}

			// With the method that we've employed, we are only able to cache a single edge traversal.
			// To mitigate this, if the algorithm is stuck in the previous node, we change the node's value to -1.
			// This will trigger the algorithm to traverse a new "random" node in the "visit_node" lambda.
			if (node == pEdge->from)
			{
				pEdge->from = static_cast<size_t>(-1);
			}

			// This is an optimization; remove all edges that were visited for faster iteration time in the step above.
			// Cater for edge cases where the graph only has 1 node and no-edges.
			if (edges.size())
			{
				auto removePredicate = [node](graph_edge const& e) -> bool { return e.to == node; };
				auto it = std::remove_if(edges.begin(), edges.end(), removePredicate);
				edges.erase(it, edges.end());
			}

			// Returning false tells the algorithm that this is a node with no edges and can be pushed into the result list.
			// Also allows us to remove the node from the vertex list.
			return false;
		};

		auto visit_node = [&](size_t& node) -> void
		{
			// This is the edges cache. 
			graph_edge e{ static_cast<size_t>(-1), static_cast<size_t>(-1) };

			// Loop through all of edges in the edges list until we've reached a node with no edge.
			while (loop_edges(node, &e));
			// Get the element for the vertex list.
			auto element = vertices.element_at_bucket(node);
			// Add the element into the result list.
			result.emplace(*element.value());
			// Remove the element from the copy of the vertex list.
			vertices.erase(*element.value());
			// Traverse back up the graph.
			node = e.from;

			// When the node is set to -1 in "loop_edges" lambda, we fetch a new node from the vertex list.
			if (node == -1 && vertices.size())
			{
				node = vertices.bucket(*vertices.begin());
			}
		};

		// Topological sort is only possible when the graph is acyclic.
		// This means any of the vertices in the vertex list do not depend on themself.
		if (is_acyclic())
		{
			// Pick a "random" vertex from the vertices array so that we can start traversing the graph.
			// "Node" and "Vertex" used interchangeably within the context of this function.
			size_t node = vertices.bucket(*vertices.begin());

			// Continuously loop through the vertices until all of them have been visited.
			while (vertices.length())
			{
				visit_node(node);
			}

			// We reverse the contents of the array so that nodes that have no/minimal dependencies appear first in the array.
			std::reverse(result.begin(), result.end());
		}

		return result;
	}
};

}

#endif // !LIB_DAG_H
