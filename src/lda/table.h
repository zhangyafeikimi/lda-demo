// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// count table and tables
//

#ifndef SRC_LDA_TABLE_H_
#define SRC_LDA_TABLE_H_

#include <assert.h>
#include <algorithm>
#include <vector>

enum {
  kHashTable = 1,
  kSparseTable,
  kDenseTable,
  kArrayBufTable,
};

template <class T>
class ITable {
 private:
  ITable(const ITable& right);
  ITable& operator=(const ITable& right);

 public:
  ITable() {}
  virtual ~ITable() {}
  virtual T Inc(int id, T count) = 0;
  virtual T Dec(int id, T count) = 0;
  virtual T Count(int id) const = 0;
  virtual int NextNonZeroCountIndex(int index) const = 0;
  virtual int Size() const = 0;
  virtual int GetId(int index) const = 0;
  virtual T GetCount(int index) const = 0;
};

template <class T>
class DenseTable : public ITable<T> {
 private:
  std::vector<T> storage_;

 public:
  DenseTable() {}

  void Init(int size) { storage_.resize(size); }

  virtual T Inc(int id, T count) {
    T& r = storage_[id];
    r += count;
    return r;
  }

  virtual T Dec(int id, T count) {
    T& r = storage_[id];
    r -= count;
    assert(r >= 0);
    return r;
  }

  virtual T Count(int id) const { return storage_[id]; }

  virtual int NextNonZeroCountIndex(int index) const {
    const int size = (int)storage_.size();
    if (index >= size) {
      return size;
    }
    while (storage_[index] == 0) {
      index++;
      if (index >= size) {
        return size;
      }
    }
    return index;
  }

  virtual int Size() const { return (int)storage_.size(); }

  virtual int GetId(int index) const { return index; }

  virtual T GetCount(int index) const { return storage_[index]; }

  T& operator[](int id) { return storage_[id]; }

  T operator[](int id) const { return storage_[id]; }
};

template <class T>
class ArrayBufTable : public ITable<T> {
 private:
  T* storage_;
  int size_;

 public:
  ArrayBufTable(T* storage, int size) : storage_(storage), size_(size) {}

  virtual T Inc(int id, T count) {
    T& r = storage_[id];
    r += count;
    return r;
  }

  virtual T Dec(int id, T count) {
    T& r = storage_[id];
    r -= count;
    assert(r >= 0);
    return r;
  }

  virtual T Count(int id) const { return storage_[id]; }

  virtual int NextNonZeroCountIndex(int index) const {
    if (index >= size_) {
      return size_;
    }

    while (storage_[index] == 0) {
      index++;
      if (index >= size_) {
        return size_;
      }
    }
    return index;
  }

  virtual int Size() const { return size_; }

  virtual int GetId(int index) const { return index; }

  virtual T GetCount(int index) const { return storage_[index]; }
};

template <class T>
class SparseTable : public ITable<T> {
 private:
  struct IdCount {
    int id;
    T count;
  };

  struct IdCountCompare {
    bool operator()(const IdCount& a, const IdCount& b) const {
      return a.id < b.id;
    }
    bool operator()(const IdCount& a, int b) const { return a.id < b; }
    bool operator()(int a, const IdCount& b) const { return a < b.id; }
  };

  const IdCountCompare compare_;
  std::vector<IdCount> storage_;

 public:
  SparseTable() : compare_() {}

  virtual T Inc(int id, T count) {
    typename std::vector<IdCount>::iterator it =
        std::lower_bound(storage_.begin(), storage_.end(), id, compare_);
    if (it != storage_.end() && it->id == id) {
      it->count += count;
      return it->count;
    } else {
      IdCount target = {id, count};
      storage_.insert(it, target);
      return count;
    }
  }

  virtual T Dec(int id, T count) {
    typename std::vector<IdCount>::iterator it =
        std::lower_bound(storage_.begin(), storage_.end(), id, compare_);
    if (it != storage_.end() && it->id == id) {
      assert(it->count >= count);
      it->count -= count;
      if (it->count == 0) {
        storage_.erase(it);
        return 0;
      } else {
        return it->count;
      }
    } else {
      assert(0);
      return -1;
    }
  }

  virtual T Count(int id) const {
    typename std::vector<IdCount>::const_iterator it =
        std::lower_bound(storage_.begin(), storage_.end(), id, compare_);
    if (it != storage_.end() && it->id == id) {
      assert(it->count > 0);
      return it->count;
    }
    return 0;
  }

  virtual int NextNonZeroCountIndex(int index) const { return index; }

  virtual int Size() const { return (int)storage_.size(); }

  virtual int GetId(int index) const { return storage_[index].id; }

  virtual T GetCount(int index) const { return storage_[index].count; }
};

static const int prime_list[] = {
    13u,     23u,     53u,     97u,      193u,     389u,   769u,
    1543u,   3079u,   6151u,   12289u,   24593u,   49157u, 98317u,
    196613u, 393241u, 786433u, 1572869u, 3145739u,
    // 6291469u,
    // 12582917u,
    // 25165843u,
    // 50331653u,
    // 100663319u,
    // 201326611u,
    // 402653189u,
    // 805306457u,
    // 1610612741u,
    // 3221225473u,
    // 4294967291u,
};

static const int prime_list_size = sizeof(prime_list) / sizeof(prime_list[0]);

inline int NextPrime(int n) {
  const int* first = prime_list;
  const int* last = prime_list + prime_list_size;
  const int* pos = std::lower_bound(first, last, n);
  return pos == last ? *(last - 1) : *pos;
}

template <class T>
class HashTable : public ITable<T> {
 private:
  enum {
    kUsed,
    kEmpty,
    kDeleted,
  };

  struct Item {
    int id;
    T count;
    int flag;
    Item() : flag(kEmpty) {}
  };

  int used_;
  int deleted_;
  std::vector<Item> storage_;

 private:
  static int _FindIndex(int id, const std::vector<Item>& storage,
                        int storage_size) {
    int pos = (unsigned int)id % storage_size;
    for (;;) {
      const Item& item = storage[pos];
      if (item.flag == kUsed || item.flag == kDeleted) {
        if (item.id == id) {
          // reuse previous slot
          return pos;
        }
      } else if (item.flag == kEmpty) {
        // new slot
        return pos;
      }

      // linear probe
      pos++;
      if (pos >= storage_size) {
        pos -= storage_size;
      }
    }
    assert(0);
    return -1;
  }

  int _FindIndex(int id) const {
    return _FindIndex(id, storage_, (int)storage_.size());
  }

  void _Rehash() {
    const int storage_size = (int)storage_.size();
    int size = used_ + deleted_;
    size = size + (size >> 1);  // * 1.5
    if (size <= storage_size) {
      return;
    }

    int new_storage_size = NextPrime(used_ << 1);
    std::vector<Item> new_storage(new_storage_size);

    for (int i = 0; i < storage_size; i++) {
      const Item& item = storage_[i];
      if (item.flag == kUsed) {
        int index = _FindIndex(item.id, new_storage, new_storage_size);
        Item& new_item = new_storage[index];
        new_item.flag = kUsed;
        new_item.id = item.id;
        new_item.count = item.count;
      }
    }

    deleted_ = 0;
    new_storage.swap(storage_);
  }

 public:
  HashTable() : used_(0), deleted_(0) { storage_.resize(prime_list[0]); }

  virtual ~HashTable() {}

  virtual T Inc(int id, T count) {
    int pos = _FindIndex(id);
    Item& item = storage_[pos];
    if (item.flag == kUsed) {
      item.count += count;
    } else if (item.flag == kDeleted) {
      item.flag = kUsed;
      item.id = id;
      item.count = count;
      used_++;
      deleted_--;
    } else {
      item.flag = kUsed;
      item.id = id;
      item.count = count;
      used_++;
      _Rehash();
    }
    return item.count;
  }

  virtual T Dec(int id, T count) {
    int pos = _FindIndex(id);
    Item& item = storage_[pos];
    if (item.flag == kUsed) {
      assert(item.count >= count);
      item.count -= count;
      if (item.count == 0) {
        item.flag = kDeleted;
        used_--;
        deleted_++;
      }
      return item.count;
    } else {
      assert(0);
      return -1;
    }
  }

  virtual T Count(int id) const {
    int pos = _FindIndex(id);
    const Item& item = storage_[pos];
    if (item.flag == kUsed) {
      assert(item.count > 0);
      return item.count;
    }
    return 0;
  }

  virtual int NextNonZeroCountIndex(int index) const {
    const int size = (int)storage_.size();
    if (index >= size) {
      return size;
    }

    while (storage_[index].flag != kUsed) {
      index++;
      if (index >= size) {
        return size;
      }
    }
    return index;
  }

  virtual int Size() const { return (int)storage_.size(); }

  virtual int GetId(int index) const {
    assert(storage_[index].flag == kUsed);
    return storage_[index].id;
  }

  virtual T GetCount(int index) const {
    assert(storage_[index].flag == kUsed);
    return storage_[index].count;
  }
};

template <class T>
class Table {
 private:
  typedef ITable<T> ITableT;
  typedef DenseTable<T> DenseTableT;
  typedef ArrayBufTable<T> ArrayBufTableT;
  typedef SparseTable<T> SparseTableT;
  typedef HashTable<T> HashTableT;
  ITableT* impl_;

 public:
  Table() : impl_(NULL) {}

  Table(const Table& right) : impl_(NULL) {
    // disable actual copy
    assert(right.impl_ == NULL);
  }

  Table& operator=(const Table& right) {
    // disable actual copy
    assert(impl_ == NULL);
    assert(right.impl_ == NULL);
    return *this;
  }

  ~Table() { delete impl_; }

  void InitDense(int size) {
    DenseTableT* hist = new DenseTableT();
    hist->Init(size);
    impl_ = hist;
  }

  void InitArrayBuf(T* storage, int size) {
    impl_ = new ArrayBufTableT(storage, size);
  }

  void InitSparse() { impl_ = new SparseTableT(); }

  void InitHash() { impl_ = new HashTableT(); }

 public:
  class const_iterator {
   private:
    const ITableT* impl_;
    int index_;

   public:
    const_iterator(const ITableT* impl, int index)
        : impl_(impl), index_(index) {}

    int id() const { return impl_->GetId(index_); }

    T count() const { return impl_->GetCount(index_); }

    bool operator==(const const_iterator& right) const {
      return impl_ == right.impl_ && index_ == right.index_;
    }

    bool operator!=(const const_iterator& right) const {
      return !(impl_ == right.impl_ && index_ == right.index_);
    }

    const_iterator& operator++() {
      index_++;
      index_ = impl_->NextNonZeroCountIndex(index_);
      return *this;
    }
  };

  const_iterator begin() const {
    return const_iterator(impl_, impl_->NextNonZeroCountIndex(0));
  }

  const_iterator end() const { return const_iterator(impl_, impl_->Size()); }

  class __Proxy {
   private:
    ITableT* impl_;
    int id_;

   public:
    __Proxy(ITableT* impl, int id) : impl_(impl), id_(id) {}

    T operator++() { return impl_->Inc(id_, 1); }

    T operator+=(T count) { return impl_->Inc(id_, count); }

    T operator--() { return impl_->Dec(id_, 1); }

    T operator-=(T count) { return impl_->Dec(id_, count); }

    operator T() { return impl_->Count(id_); }
  };

  __Proxy operator[](int id) { return __Proxy(impl_, id); }

  T operator[](int id) const { return impl_->Count(id); }
};

template <class T>
class Tables {
 public:
  typedef Table<T> TableT;

 private:
  Tables(const Tables& right);
  Tables& operator=(const Tables& right);

 private:
  int d1_;
  int d2_;
  std::vector<TableT> matrix_;
  std::vector<int> array_buf_;

 public:
  Tables() : d1_(0), d2_(0) {}

  int d1() const { return d1_; }

  int d2() const { return d2_; }

  void Init(int d1, int d2, int type) {
    d1_ = d1;
    d2_ = d2;
    matrix_.resize(d1);

    switch (type) {
      case kHashTable:
        for (int i = 0; i < d1_; i++) {
          matrix_[i].InitHash();
        }
        break;
      case kSparseTable:
        for (int i = 0; i < d1_; i++) {
          matrix_[i].InitSparse();
        }
        break;
      case kDenseTable:
        for (int i = 0; i < d1_; i++) {
          matrix_[i].InitDense(d2_);
        }
        break;
      case kArrayBufTable:
        array_buf_.resize(d1_ * d2_);
        for (int i = 0; i < d1_; i++) {
          matrix_[i].InitArrayBuf(&array_buf_[0] + i * d2_, d2_);
        }
        break;
    }
  }

  TableT& operator[](int i) { return matrix_[i]; }

  const TableT& operator[](int i) const { return matrix_[i]; }
};

typedef Table<int> IntTable;
typedef Tables<int> IntTables;

#endif  // SRC_LDA_TABLE_H_
