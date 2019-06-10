#pragma once

#include <string>
#include <memory>
#include <cstddef>
#include <iterator>
#include <vector>
#include <utility>
#include <string_view>
#include <cassert>
#include <algorithm>
#include <stdexcept>

template <typename T, template<typename...> typename A = std::allocator>
class StringMap
{
	struct Node;
	typedef Node* NodeP;

public:
	typedef std::string key_type;
	typedef T mapped_type;
	typedef T value_type;
	//typedef std::pair<std::string, T> value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;
	struct iterator;
	struct const_iterator;

	StringMap() noexcept;
	StringMap(const StringMap&);
	StringMap(StringMap&&) noexcept;
	StringMap& operator=(const StringMap&);
	StringMap& operator=(StringMap&&) noexcept;
	~StringMap();
	void swap(StringMap&) noexcept;
	void clear();

	std::size_t size() const;

	struct iterator
	{
		typedef T value_type;
		typedef T* pointer;
		typedef T& reference;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;
		typedef std::bidirectional_iterator_tag iterator_category;

		iterator() = default;

		T& operator*() { return *ptr(); }
		T* operator->() { return ptr(); }
		std::string key() const;
		iterator& operator++() { node = node->Next(); return *this; }
		iterator& operator--() { node = node->Prev(); return *this; }
		iterator operator++(int) { iterator tmp = *this; node = node->Next(); return tmp; }
		iterator operator--(int) { iterator tmp = *this; node = node->Prev(); return tmp; }
		bool operator==(const iterator& other) const { return node == other.node; }
		bool operator!=(const iterator& other) const { return node != other.node; }
	private:
		iterator(NodeP p) : node(p) {}
		T* ptr() { return node->item; }
		NodeP node = nullptr;
	friend
		class StringMap;
	friend
		struct const_iterator;
	};
	struct const_iterator
	{
		typedef const T value_type;
		typedef const T* pointer;
		typedef const T& reference;
		typedef std::ptrdiff_t difference_type;
		typedef std::size_t size_type;
		typedef std::bidirectional_iterator_tag iterator_category;

		const_iterator() = default;
		const_iterator(iterator i) : node(i.node) {}

		const T& operator*() { return *ptr(); }
		const T* operator->() { return ptr(); }
		std::string key() const;
		const_iterator& operator++() { node = node->Next(); return *this; }
		const_iterator& operator--() { node = node->Prev(); return *this; }
		const_iterator operator++(int) { const_iterator tmp = *this; node = node->Next(); return tmp; }
		const_iterator operator--(int) { const_iterator tmp = *this; node = node->Prev(); return tmp; }
		bool operator==(const const_iterator& other) const { return node == other.node; }
		bool operator!=(const const_iterator& other) const { return node != other.node; }
	private:
		const_iterator(NodeP p) : node(p) {}
		const T* ptr() { return node->item; }
		NodeP node = nullptr;
	friend
		class StringMap;
	};

	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	iterator begin();
	iterator end() { return {&head}; }

	const_iterator begin() const;
	const_iterator end() const { return const_iterator{&head}; }
	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }

	reverse_iterator rbegin() { return reverse_iterator{end()}; }
	reverse_iterator rend() { return reverse_iterator{begin()}; }

	const_reverse_iterator rbegin() const { return const_reverse_iterator{end()}; }
	const_reverse_iterator rend() const { return const_reverse_iterator{begin()}; }
	const_reverse_iterator crbegin() const { return const_reverse_iterator{end()}; }
	const_reverse_iterator crend() const { return const_reverse_iterator{begin()}; }

	struct search_result_type
	{
		bool exact_match;
		std::size_t count;
		iterator begin;
		iterator end;
	};
	struct const_search_result_type
	{
		bool exact_match;
		std::size_t count;
		const_iterator begin;
		const_iterator end;
	};

	struct insert_result_type
	{
		bool collision;
		iterator iter;
	};

	insert_result_type insert(std::string_view key, const T& val);
	insert_result_type insert_or_overwrite(std::string_view key, const T& val);
	insert_result_type insert_or_overwrite(std::string_view key, T&& val);

	template<typename... Args>
	insert_result_type emplace(std::string_view key, const Args&... args);
	template<typename... Args>
	insert_result_type emplace_or_overwrite(std::string_view& key, Args&&... args);

	std::size_t count(std::string_view key) const;
	std::size_t count_exact(std::string_view key) const;
	search_result_type find(std::string_view key);
	const_search_result_type find(std::string_view key) const;

	iterator erase(iterator);

	T& operator[](std::string_view key);
	const T& operator[](std::string_view key) const;

private:
	struct Node {
		Node();
		char ch;
		std::size_t leaf_count;
		NodeP leafs[256];
		NodeP parent;
		T* item;
		NodeP Next();
		NodeP Prev();
		NodeP After(char);
		NodeP Before(char);
		void AddPropagate(int);
		std::string Key();
	};
	typedef A<Node> NodeAlloc;
	typedef A<T> ItemAlloc;

	NodeAlloc na;
	ItemAlloc ia;

	mutable Node head;

	void copy_from(const StringMap&);

	std::pair<NodeP, bool> create_or_find(std::string_view); // <node, did-create>

	NodeP find_exact(std::string_view) const;

	void recursive_tree_delete(NodeP ptr);
	void recursive_tree_infuse(NodeP src, NodeP& dst, NodeP par, bool owerwrite);
};

template <typename T, template<typename...> typename A>
StringMap<T, A>::StringMap() noexcept
{
	head.ch = 0;
	head.parent = &head;
}

template <typename T, template<typename...> typename A>
StringMap<T, A>::StringMap(const StringMap& other)
	: StringMap()
{
	copy_from(other);
}

template <typename T, template<typename...> typename A>
StringMap<T, A>::StringMap(StringMap&& other) noexcept
	: StringMap()
{
	swap(other);
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::operator=(const StringMap& other) -> StringMap&
{
	clear();
	copy_from(other);
	return *this;
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::operator=(StringMap&& other) noexcept -> StringMap&
{
	swap(other);
	return *this;
}

template <typename T, template<typename...> typename A>
StringMap<T, A>::~StringMap()
{
	clear();
}

template <typename T, template<typename...> typename A>
StringMap<T, A>::Node::Node()
{
	leaf_count = 0;
	for (auto& p : leafs)
		p = nullptr;
	item = nullptr;
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::Node::Next() -> NodeP
{
	NodeP p = nullptr;
	for (auto l : leafs)
	{
		if (l)
		{
			p = l;
			break;
		}
	}
	if (!p)
	{
		NodeP par = parent;
		if (par->parent == par)
			return par;
		return par->After(ch);
	}
	if (p->item)
		return p;
	else
		return p->Next();
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::Node::Prev() -> NodeP
{
	return nullptr;
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::Node::After(char c) -> NodeP
{
	NodeP p = nullptr;
	int i = (unsigned char)c;
	for (++i;i<256;++i)
	{
		if (leafs[i])
		{
			p = leafs[i];
			break;
		}
	}
	if (!p)
	{
		NodeP par = parent;
		if (par->parent == par)
			return par;
		return par->After(ch);
	}
	if (p->item)
		return p;
	else
		return p->Next();
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::Node::Before(char c) -> NodeP
{
	NodeP p = nullptr;
	int i = (unsigned char)c;
	for (--i; i >= 0; --i)
	{
		if (leafs[i])
		{
			p = leafs[i];
			break;
		}
	}
	if (!p || p->leaf_count == 0)
	{
		NodeP par = parent;
		if (par->parent == par)
			return par;
		return par->Before(ch);
	}
	if (p->item && p->leaf_count==1)
		return p;

	while (true)
	{
		for (i=255; i >= 0; --i)
		{
			if (p->leafs[i])
			{
				p = leafs[i];
				break;
			}
		}
		if (p->item && p->leaf_count == 1)
			return p;
	}
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::Node::AddPropagate(int amount) -> void
{
	leaf_count += amount;
	NodeP par = parent;
	if (par->parent != par)
		par->AddPropagate(amount);
}

template <typename T, template<typename...> typename A>
void StringMap<T, A>::swap(StringMap& other) noexcept
{
	using std::swap;
	swap( head.leaf_count, other.head.leaf_count );
	swap( head.leafs[0], other.head.leafs[0] );

	if (head.leafs[0])
		head.leafs[0]->parent = &head;
	if (other.head.leafs[0])
		other.head.leafs[0]->parent = &other.head;
}

template <typename T, template<typename...> typename A>
void StringMap<T, A>::recursive_tree_delete(NodeP p)
{
	if (!p) return;
	if (p->item)
	{
		p->item->~T();
		ia.deallocate(p->item, 1);
	}
	for (NodeP l : p->leafs)
	{
		recursive_tree_delete(l);
	}
	p->~Node();
	na.deallocate(p, 1);
}

template <typename T, template<typename...> typename A>
void StringMap<T, A>::clear()
{
	recursive_tree_delete(head.leafs[0]);
	head.leafs[0] = nullptr;
	head.leaf_count = 0;
}

template <typename T, template<typename...> typename A>
std::size_t StringMap<T, A>::size() const
{
	return head.leaf_count;
}

template <typename T, template<typename...> typename A>
std::string StringMap<T, A>::Node::Key()
{
	std::string str;
	NodeP p = this;
	while (true)
	{
		NodeP par = p->parent;
		if (par->parent == par) break;
		str += p->ch;
		p = par;
	}
	std::reverse(str.begin(), str.end());
	return str;
}

template <typename T, template<typename...> typename A>
std::string StringMap<T, A>::iterator::key() const
{
	return node->Key();
}

template <typename T, template<typename...> typename A>
std::string StringMap<T, A>::const_iterator::key() const
{
	return node->Key();
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::begin() -> iterator
{
	NodeP p = &head;
	return {p->Next()};
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::begin() const -> const_iterator
{
	NodeP p = &head;
	return {p->Next()};
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::insert(std::string_view key, const T& val) -> insert_result_type
{
	auto [ptr, did_create] = create_or_find(key);
	if (!did_create && ptr->item)
		return {true, {ptr}};

	ptr->item = ia.allocate(1);
	new (ptr->item) T(val);
	ptr->AddPropagate(1);
	return {false, {ptr}};
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::insert_or_overwrite(std::string_view key, const T& val) -> insert_result_type
{
	auto [ptr, did_create] = create_or_find(key);
	if (!did_create && ptr->item)
	{
		*ptr->item = val;
		return {true, {ptr}};
	}

	ptr->item = ia.allocate(1);
	new (ptr->item) T(val);
	ptr->AddPropagate(1);
	return {false, {ptr}};
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::insert_or_overwrite(std::string_view key, T&& val) -> insert_result_type
{
	auto [ptr, did_create] = create_or_find(key);
	if (!did_create && ptr->item)
	{
		*ptr->item = std::move(val);
		return {true, {ptr}};
	}

	ptr->item = ia.allocate(1);
	new (ptr->item) T(std::move(val));
	ptr->AddPropagate(1);
	return {false, {ptr}};
}

template <typename T, template<typename...> typename A>
T& StringMap<T, A>::operator[](std::string_view key)
{
	return *insert_or_overwrite(key, T{}).iter;
}

template <typename T, template<typename...> typename A>
const T& StringMap<T, A>::operator[](std::string_view key) const
{
	NodeP p = find_exact(key);
	if (p && p->item)
		return *p->item;
	else
		throw std::range_error{"key not found"};
}

template <typename T, template<typename...> typename A>
template<typename... Args>
auto StringMap<T, A>::emplace(std::string_view key, const Args&... args) -> insert_result_type
{
	auto [ptr, did_create] = create_or_find(key);
	if (!did_create && ptr->item)
		return {true, {ptr}};

	ptr->item = ia.allocate(1);
	new (ptr->item) T(args...);
	ptr->AddPropagate(1);
	return {false, {ptr}};
}

template <typename T, template<typename...> typename A>
template<typename... Args>
auto StringMap<T, A>::emplace_or_overwrite(std::string_view& key, Args&&... args) -> insert_result_type
{
	auto [ptr, did_create] = create_or_find(key);
	if (!did_create && ptr->item)
		return {true, {ptr}};

	ptr->item = ia.allocate(1);
	new (ptr->item) T(std::forward<Args>(args)...);
	ptr->AddPropagate(1);
	return {false, {ptr}};
}

template <typename T, template<typename...> typename A>
std::size_t StringMap<T, A>::count(std::string_view key) const
{
	NodeP p = head.leafs[0];
	for (auto c : key)
	{
		if (!p) return 0;
		int i = (unsigned char)c;
		p = p->leafs[i];
	}
	return p ? p->leaf_count : 0;
}

template <typename T, template<typename...> typename A>
std::size_t StringMap<T, A>::count_exact(std::string_view key) const
{
	NodeP p = head.leafs[0];
	for (auto c : key)
	{
		if (!p) return 0;
		int i = (unsigned char)c;
		p = p->leafs[i];
	}
	return p && p->item;
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::find(std::string_view key) -> search_result_type
{
	NodeP p = head.leafs[0];
	for (auto c : key)
	{
		if (!p) return {false,0,{},{}};
		int i = (unsigned char)c;
		p = p->leafs[i];
	}
	if (!p) return {false,0,{},{}};
	search_result_type srt;
	srt.exact_match = p->item;
	srt.count = p->leaf_count;
	srt.end = iterator{p->parent->After(p->ch)};
	if (!p->item) p = p->Next();
	srt.begin = iterator{p};

	return srt;
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::find(std::string_view key) const -> const_search_result_type
{
	NodeP p = head.leafs[0];
	for (auto c : key)
	{
		if (!p) return {false,0,{},{}};
		int i = (unsigned char)c;
		p = p->leafs[i];
	}
	if (!p) return {false,0,{},{}};
	const_search_result_type srt;
	srt.exact_match = p->item;
	srt.count = p->leaf_count;
	srt.begin = const_iterator{p};
	srt.end = const_iterator{p->parent->After(p->ch)};
	return srt;
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::erase(iterator itr) -> iterator
{
	NodeP p = itr.node;
	NodeP nxt = p->Next();
	if (p->item)
	{
		p->item->~T();
		ia.deallocate(p->item, 1);
		p->AddPropagate(-1);
		if (!p->leaf_count)
			recursive_tree_delete(p);
	}
	return {nxt};
}

template <typename T, template<typename...> typename A>
void StringMap<T, A>::recursive_tree_infuse(NodeP src, NodeP& dst, NodeP par, bool owerwrite)
{
	if (!src)
		return;

	if (!dst)
	{
		dst = na.allocate(1);
		new (dst) Node;
		dst->parent = par;
		dst->ch = src->ch;
	}

	if (src->item)
	{
		if (dst->item && owerwrite)
		{
			*dst->item = *src->item;
		}
		else if (!dst->item)
		{
			dst->item = ia.allocate(1);
			new (dst->item) T(*src->item);
		}
	} else {
		dst->item = nullptr;
	}

	for (int i=0; i<256; ++i)
	{
		recursive_tree_infuse( src->leafs[i], dst->leafs[i], dst, owerwrite );
	}
}

template <typename T, template<typename...> typename A>
void StringMap<T, A>::copy_from(const StringMap& other)
{
	recursive_tree_infuse(other.head.leafs[0], head.leafs[0], &head, false);
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::create_or_find(std::string_view key) -> std::pair<NodeP, bool>
{
	bool created = false;

	NodeP p = head.leafs[0];
	if (!p)
	{
		p = head.leafs[0] = na.allocate(1);
		new (p) Node;
		p->parent = &head;
		created = true;
	}

	for (auto c : key)
	{
		int i = (unsigned char)c;
		if (p->leafs[i])
		{
			p = p->leafs[i];
		} else {
			p->leafs[i] = na.allocate(1);
			new (p->leafs[i]) Node;
			p->leafs[i]->parent = p;
			p = p->leafs[i];
			p->ch = c;
			created = true;
		}
	}

	return {p, created};
}

template <typename T, template<typename...> typename A>
auto StringMap<T, A>::find_exact(std::string_view key) const -> NodeP
{
	NodeP p = head.leafs[0];
	for (auto c : key)
	{
		if (!p) return nullptr;
		int i = (unsigned char)c;
		p = p->leafs[i];
	}
	return p;
}

template class StringMap<int>;

void test_string_map()
{
	using namespace std;

	StringMap<int> smi;

	smi[ "lala" ] =  7;
	smi[ "lau"  ] = 11;
	smi[ "kok"  ] = 33;
	smi[ "kote" ] = 14;

	auto itr = smi.begin();
	while (itr != smi.end())
	{
		cout << itr.key() << " : " << *itr << endl;
		//auto tst = itr; ++tst; --tst;
		//assert (itr == tst);
		++itr;
	}

	auto rb = smi.rbegin();

	cout << smi.count("la") << endl;
	auto res = smi.find("la");
	cout << res.count << " - " << boolalpha << res.exact_match << endl;
	for (; res.begin != res.end; ++res.begin)
		cout << res.begin.key() << " : " << *res.begin << endl;

}

