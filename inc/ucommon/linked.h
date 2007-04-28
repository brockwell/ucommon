#ifndef _UCOMMON_LINKED_H_
#define	_UCOMMON_LINKED_H_

#ifndef	_UCOMMON_OBJECT_H_
#include <ucommon/object.h>
#endif

NAMESPACE_UCOMMON

class OrderedObject;

class __EXPORT LinkedObject
{
protected:
	friend class OrderedIndex;
	friend class LinkedRing;
	friend class NamedObject;

	LinkedObject *next;

	LinkedObject();
	LinkedObject(LinkedObject **root);
	virtual ~LinkedObject();

	virtual void release(void);

public:
	void enlist(LinkedObject **root);
	void delist(LinkedObject **root);

	bool isMember(LinkedObject *list) const;

	static void purge(LinkedObject *root);

	static unsigned count(LinkedObject *root);

	inline LinkedObject *getNext(void) const
		{return next;};
};

#define nil ((void*)(NULL))

class __EXPORT OrderedIndex
{
protected:
	friend class OrderedObject;
	friend class LinkedList;
	friend class NamedObject;

	OrderedObject *head, *tail;

public:
	OrderedIndex();
	~OrderedIndex();

	LinkedObject *find(unsigned index) const;

	unsigned count(void) const;

	void purge(void);

	LinkedObject **index(void) const;

	inline LinkedObject *begin(void) const
		{return (LinkedObject*)(head);};

	inline LinkedObject *end(void) const
		{return (LinkedObject*)(tail);};

	inline LinkedObject *operator*() const
		{return (LinkedObject*)(head);};
};

class __EXPORT OrderedObject : public LinkedObject
{
protected:
	friend class LinkedList;
	friend class OrderedIndex;

	OrderedObject(OrderedIndex *root);
	OrderedObject();

public:
	void enlist(OrderedIndex *root);
	void delist(OrderedIndex *root);

	inline OrderedObject *getNext(void) const
		{return static_cast<OrderedObject *>(LinkedObject::getNext());};
};

class __EXPORT NamedObject : public OrderedObject
{
protected:
	char *id;

	NamedObject();
    NamedObject(NamedObject **root, char *id, unsigned max = 1);
	NamedObject(OrderedIndex *idx, char *id);
	~NamedObject();

public:
	static void purge(NamedObject **idx, unsigned max);

	static NamedObject **index(NamedObject **idx, unsigned max);

	static unsigned count(NamedObject **idx, unsigned max);

	static NamedObject *find(NamedObject *root, const char *id);

	static NamedObject *map(NamedObject **idx, const char *id, unsigned max);

	static NamedObject *skip(NamedObject **idx, NamedObject *current, unsigned max);

	static unsigned keyindex(const char *id, unsigned max);

	static NamedObject **sort(NamedObject **list, size_t count = 0);

	inline NamedObject *getNext(void) const
		{return static_cast<NamedObject*>(LinkedObject::getNext());};

	inline char *getId(void) const
		{return id;};

	virtual bool compare(const char *cmp) const;

	inline bool operator==(const char *cmp) const
		{return compare(cmp);};

	inline bool operator!=(const char *cmp) const
		{return !compare(cmp);};
};

class __EXPORT NamedTree : public NamedObject
{
protected:
	NamedTree *parent;
	OrderedIndex child;

	NamedTree(char *id = NULL);
	NamedTree(NamedTree *parent, char *id);
	virtual ~NamedTree();

	void purge(void);

public:
	NamedTree *find(const char *tag) const;

	NamedTree *path(const char *path) const;

	NamedTree *leaf(const char *tag) const;

	NamedTree *getChild(const char *tag) const;

	NamedTree *getLeaf(const char *tag) const;

	inline NamedTree *getFirst(void) const
		{return static_cast<NamedTree *>(child.begin());};

	inline NamedTree *getParent(void) const
		{return parent;};

	inline OrderedIndex *getIndex(void) const
		{return const_cast<OrderedIndex*>(&child);};

	inline operator bool() const
		{return (id != NULL);};

	inline bool operator!() const
		{return (id == NULL);};

	void setId(char *id);

	void remove(void);

	inline bool isLeaf(void) const
		{return (child.begin() == NULL);};
};

class __EXPORT NamedList : public NamedObject
{
protected:
	friend class objmap;

	NamedObject **keyroot;
	unsigned keysize;

	NamedList(NamedObject **root, char *id, unsigned max);
	virtual ~NamedList();

public:
	void delist(void);
};		

class __EXPORT LinkedList : public OrderedObject
{
protected:
	LinkedList *prev;
	OrderedIndex *root;

	LinkedList(OrderedIndex *root);
	LinkedList();

	virtual ~LinkedList();

public:
	void delist(void);
	void enlist(OrderedIndex *root);

	inline bool isHead(void) const
		{return root->head == (OrderedObject *)this;};

	inline bool isTail(void) const
		{return root->tail == (OrderedObject *)this;};

	inline LinkedList *getPrev(void) const
		{return prev;};

	inline LinkedList *getNext(void) const
		{return static_cast<LinkedList*>(LinkedObject::getNext());};
};
	
class __EXPORT objmap
{
protected:
	NamedList *object;

public:
	objmap(NamedList *obj);

	void operator++();

	objmap &operator=(NamedList *root);

	unsigned count(void) const;

	void begin(void) const;
};

template <class T, class O=NamedObject>
class named_value : public object_value<T, O>
{
public:
	inline named_value(LinkedObject **root, char *i) 
		{LinkedObject::enlist(root); O::id = i;};

	inline void operator=(T v)
		{set(v);};

	inline static named_value find(named_value *base, const char *id)
		{return static_cast<named_value *>(NamedObject::find(base, id));};
};

template <class T, class O=OrderedObject>
class linked_value : public object_value<T, O>
{
public:
	inline linked_value() {};

	inline linked_value(LinkedObject **root)
		{LinkedObject::enlist(root);};

	inline linked_value(OrderedIndex *index) 
		{O::enlist(index);};

	inline linked_value(LinkedObject **root, T v) 
		{LinkedObject::enlist(root); set(v);};

	inline linked_value(OrderedIndex *index, T v)
 		{O::enlist(index); set(v);};

	inline void operator=(T v)
		{set(v);};
};	

template <class T>
class linked_pointer
{
private:
	T *ptr;

public:
	inline linked_pointer(T *p)
		{ptr = p;};

	inline linked_pointer(const linked_pointer &p)
		{ptr = p.ptr;};

	inline linked_pointer(LinkedObject *p)
		{ptr = static_cast<T*>(p);};

	inline linked_pointer(OrderedIndex *a)
		{ptr = static_cast<T*>(a->begin());};

	inline linked_pointer() 
		{ptr = NULL;};

	inline void operator=(T *v)
		{ptr = v;};

	inline void operator=(linked_pointer &p)
		{ptr = p.ptr;};

	inline void operator=(OrderedIndex *a)
		{ptr = static_cast<T*>(a->begin());};

	inline void operator=(LinkedObject *p)
		{ptr = static_cast<T*>(p);};

	inline T* operator->() const
		{return ptr;};

	inline T* operator*() const
		{return ptr;};

	inline operator T*() const
		{return ptr;};

	inline void prev(void)
		{ptr = static_cast<T*>(ptr->getPrev());};

	inline void next(void)
		{ptr = static_cast<T*>(ptr->getNext());};

	inline T *getNext(void) const
		{return static_cast<T*>(ptr->getNext());};

    inline T *getPrev(void) const
        {return static_cast<T*>(ptr->getPrev());};

	inline void operator++()
		{ptr = static_cast<T*>(ptr->getNext());};

    inline void operator--()
        {ptr = static_cast<T*>(ptr->getPrev());};

	inline bool isNext(void) const
		{return (ptr->getNext() != NULL);};

	inline bool isPrev(void) const
		{return (ptr->getPrev() != NULL);};

	inline operator bool() const
		{return (ptr != NULL);};

	inline bool operator!() const
		{return (ptr == NULL);};

    inline LinkedObject **root(void) const
		{T **r = &ptr; return (LinkedObject**)r;};
};

template <class T>
class treemap : public NamedTree
{
protected:
	T value;

public:
	inline treemap(char *id = NULL) : NamedTree(id) {};
	inline treemap(treemap *parent, char *id) : NamedTree(parent, id) {};
	inline treemap(treemap *parent, char *id, T &v) :
		NamedTree(parent, id) {value = v;};

	inline T &get(void) const
		{return value;};

	inline T operator*() const
		{return value;};

	static inline T getPointer(treemap *node)
		{(node == NULL) ? NULL : node->value;};

	inline bool isAttribute(void) const
		{return (!child.begin() && value != NULL);};

	inline T getPointer(void) const
		{return value;};

	inline T getData(void) const
		{return value;};

	inline void setPointer(const T p)
		{value = p;};

	inline void set(const T &v)
		{value = v;};

	inline void operator=(const T &v)
		{value = v;};

	inline treemap *getParent(void) const
		{return static_cast<treemap*>(parent);};

	inline treemap *getChild(const char *id) const
		{return static_cast<treemap*>(NamedTree::getChild(id));};

	inline treemap *getLeaf(const char *id) const
		{return static_cast<treemap*>(NamedTree::getLeaf(id));};

	inline T getValue(const char *id) const
		{return getPointer(getLeaf(id));};

	inline treemap *find(const char *id) const
		{return static_cast<treemap*>(NamedTree::find(id));};

	inline treemap *path(const char *path) const
		{return static_cast<treemap*>(NamedTree::path(path));};

	inline treemap *leaf(const char *id) const
		{return static_cast<treemap*>(NamedTree::leaf(id));};

	inline treemap *getFirst(void) const
		{return static_cast<treemap*>(NamedTree::getFirst());};
};

template <class T, unsigned M = 177>
class keymap
{
private:
	NamedObject *idx[M];

public:
	inline ~keymap()
		{NamedObject::purge(idx, M);};

	inline NamedObject **root(void) const
		{return idx;};

	inline unsigned limit(void) const
		{return M;};

	inline T *get(const char *id) const
		{return static_cast<T*>(NamedObject::map(idx, id, M));};

	inline T *begin(void) const
		{return static_cast<T*>(NamedObject::skip(idx, NULL, M));};

	inline T *next(T *current) const
		{return static_cast<T*>(NamedObject::skip(idx, current, M));};

	inline unsigned count(void) const
		{return NamedObject::count(idx, M);};

	inline T **index(void) const
		{return NamedObject::index(idx, M);};

	inline T **sort(void) const
		{return NamedObject::sort(NamedObject::index(idx, M));};
}; 

template <class T>
class keylist : public OrderedIndex
{
public:
	inline NamedObject **root(void)
		{return static_cast<NamedObject*>(&head);};

	inline T *begin(void)
		{return static_cast<T*>(head);};

	inline T *end(void)
		{return static_cast<T*>(tail);};

	inline T *create(const char *id)
		{return new T(this, id);};

    inline T *next(LinkedObject *current)
        {return static_cast<T*>(current->getNext());};

    inline T *find(const char *id)
        {return static_cast<T*>(NamedObject::find(begin(), id));};

    inline T *operator[](unsigned index)
        {return static_cast<T*>(OrderedIndex::find(index));};

	inline T **sort(void)
		{return NamedObject::sort(index());};
};

END_NAMESPACE

#endif