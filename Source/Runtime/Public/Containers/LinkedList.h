// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

BE_NAMESPACE_BEGIN

#pragma once

class LinkNode
{
public:
    LinkNode *m_pNext;
    LinkNode *m_pPrev;

    void PushFront(LinkNode *pParent)
    {
        Append(pParent);
    }

    void PushBack(LinkNode *pParent)
    {
        Insert(pParent);
    }

    void MoveTo(LinkNode *pTo)
    {
        if (m_pNext != this) {
            pTo->m_pPrev->m_pNext = m_pNext;
            m_pNext->m_pPrev = pTo->m_pPrev;
            pTo->m_pPrev = m_pPrev;
            m_pPrev->m_pNext = pTo;
            m_pPrev = m_pNext = this;
        }
    }

    bool Empty()
    {
        return (this == m_pNext);
    }
    void Initialize()
    {
        m_pPrev = m_pNext = this;
    }
    void Append(LinkNode *pLink)
    {
        m_pNext = pLink->m_pNext;
        m_pPrev = pLink;
        pLink->m_pNext->m_pPrev = this;
        pLink->m_pNext = this;
    }

    void Insert(LinkNode *pLink)
    {
        m_pPrev = pLink->m_pPrev;
        m_pNext = pLink;
        pLink->m_pPrev->m_pNext = this;
        pLink->m_pPrev = this;
    }
    void Remove()
    {
        m_pNext->m_pPrev = m_pPrev;
        m_pPrev->m_pNext = m_pNext;
    }

};


//#define LINK_POINTER( link, type, member) (( type *)(( char *)link - ( size_t)&(( type *)0)->member))
#define LinkedList(type, member) linked_list<type, offsetof(type, member)>

template <typename TYPE, size_t offset = 0> class _linked_list
{
public:
    typedef	TYPE &reference;
    typedef	TYPE *pointer;
    typedef	TYPE value_type;
protected:
    LinkNode m_list;

public:
    class iterator : public std::iterator<std::bidirectional_iterator_tag, TYPE>
    {	// iterator for mutable list
    public:

        iterator() {}
        iterator(reference value)
        {
            m_ptr = &value;
        }

        explicit iterator(LinkNode *pLink)
        {
            m_ptr = (pointer)((char *)pLink - offset);
        }

        LinkNode* ToLinkPtr() const
        {
            return (LinkNode *)((char *)m_ptr + offset);
        }

        reference operator*()
        {	// return designated value
            return *m_ptr;
        }

        //pointer operator->( ) const
        //{	// return pointer to class object
        //	return ( &**this);
        //}

        iterator& operator++()
        {	// preincrement
            m_ptr = iterator(ToLinkPtr()->m_pNext).m_ptr;
            return (*this);
        }

        iterator operator++(int)
        {	// postincrement
            iterator _Tmp = *this;
            ++*this;
            return (_Tmp);
        }

        iterator& operator--()
        {	// predecrement
            m_ptr = iterator(ToLinkPtr()->m_pPrev).m_ptr;
            return (*this);
        }

        iterator operator--(int)
        {	// postdecrement
            iterator _Tmp = *this;
            --*this;
            return (_Tmp);
        }

        bool operator==(const iterator& _Right) const
        {	// test for iterator equality
            return (m_ptr == _Right.m_ptr);
        }

        bool operator!=(const iterator& _Right) const
        {	// test for iterator inequality
            return (!(*this == _Right));
        }

    protected:
        pointer m_ptr;
    };
    class reverse_iterator : public std::iterator<std::bidirectional_iterator_tag, TYPE>
    {	// iterator for mutable list
    public:

        reverse_iterator() {}
        reverse_iterator(reference value)
        {
            m_ptr = value;
        }

        explicit reverse_iterator(LinkNode *pLink)
        {
            m_ptr = (pointer)((char *)pLink - offset);
        }

        LinkNode* ToLinkPtr() const
        {
            return (LinkNode *)((char *)m_ptr + offset);
        }

        reference operator*()
        {	// return designated value
            return *m_ptr;
        }

        //pointer operator->( ) const
        //{	// return pointer to class object
        //	return ( &**this);
        //}

        reverse_iterator& operator++()
        {	// preincrement
            m_ptr = reverse_iterator(ToLinkPtr()->m_pPrev).m_ptr;
            return (*this);
        }

        reverse_iterator operator++(int)
        {	// postincrement
            reverse_iterator _Tmp = *this;
            ++*this;
            return (_Tmp);
        }

        reverse_iterator& operator--()
        {	// predecrement
            m_ptr = reverse_iterator(ToLinkPtr()->m_pNext).m_ptr;
            return (*this);
        }

        reverse_iterator operator--(int)
        {	// postdecrement
            reverse_iterator _Tmp = *this;
            --*this;
            return (_Tmp);
        }

        bool operator==(const reverse_iterator& _Right) const
        {	// test for iterator equality
            return (m_ptr == _Right.m_ptr);
        }

        bool operator!=(const reverse_iterator& _Right) const
        {	// test for iterator inequality
            return (!(*this == _Right));
        }

    protected:
        pointer m_ptr;
    };

    _linked_list()
    {
        m_list.Initialize();
    }

    void push_front(reference element)
    {
        iterator(element).ToLinkPtr()->Append(&m_list);
    }

    void push_back(reference element)
    {
        iterator(element).ToLinkPtr()->Insert(&m_list);
    }

    reference front()
    {
        return *iterator(m_list.m_pNext);
    }

    reference back()
    {
        return *iterator(m_list.m_pPrev);
    }

    void pop_front()
    {
        m_list.m_pNext->Remove();
    }

    void pop_back()
    {
        m_list.m_pPrev->Remove();
    }

    bool empty()
    {
        return m_list.Empty() != 0;
    }

    iterator begin()
    {
        return iterator(m_list.m_pNext);
    }

    iterator end()
    {
        return iterator(&m_list);
    }

    reverse_iterator rbegin()
    {	// return iterator for beginning of reversed mutable sequence
        return (reverse_iterator(m_list.m_pPrev));
    }

    reverse_iterator rend()
    {	// return iterator for end of reversed mutable sequence
        return reverse_iterator(&m_list);
    }

    iterator insert(iterator where, reference element)
    {
        iterator(element).ToLinkPtr()->Insert(where.ToLinkPtr());
        return iterator(element);
    }

    static iterator erase(iterator where)
    {
        (where++).ToLinkPtr()->Remove();
        return where;
    }

    static iterator iterator_to(reference element)
    {
        return iterator(element);
    }

    void clear()
    {
        m_list.Initialize();
    }
};

template <typename TYPE, size_t offset = 0> class linked_list : public _linked_list<TYPE, offset>
{
protected:
    long m_nSize;
public:
    typedef	_linked_list<TYPE, offset> Super;
    typedef typename Super::iterator iterator;
    typedef typename Super::reference reference;

    linked_list()
    {
        m_nSize = 0;
    }

    void push_front(reference element)
    {
        Super::push_front(element);
        m_nSize++;
    }

    void push_back(reference element)
    {
        Super::push_back(element);
        m_nSize++;
    }

    void pop_front()
    {
        Super::pop_front();
        m_nSize--;
    }

    void pop_back()
    {
        Super::pop_back();
        m_nSize--;
    }

    bool empty()
    {
        return m_nSize == 0;
    }

    iterator insert(iterator where, reference element)
    {
        m_nSize++;
        return Super::insert(where, element);
    }

    iterator erase(iterator where)
    {
        m_nSize--;
        return Super::erase(where);
    }

    void clear()
    {
        m_nSize = 0;
        Super::clear();
    }

    size_t size()
    {
        return m_nSize;
    }

};


BE_NAMESPACE_END
