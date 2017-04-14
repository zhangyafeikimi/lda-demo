// Copyright (c) 2015-2017 Contibutors.
// Author: Yafei Zhang (zhangyafeikimi@gmail.com)
//
// count tables
//

#ifndef TABLE_H_
#define TABLE_H_

#include <assert.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "x.h"

template <class T>
class DenseTableT {
 public:
  typedef T ElementType;
  DenseTableT() {}

  void Init(int hint_size) { storage_.resize(hint_size); }
  ElementType Inc(int id, ElementType count) { return storage_[id] += count; }
  ElementType Dec(int id, ElementType count) { return storage_[id] -= count; }
  ElementType Count(int id) const { return storage_[id]; }

  int NextNonZeroCountIndex(int index) const {
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

  int Size() const { return (int)storage_.size(); }
  int GetID(int index) const { return index; }
  ElementType GetCount(int index) const { return storage_[index]; }
  ElementType& operator[](int id) { return storage_[id]; }
  ElementType operator[](int id) const { return storage_[id]; }

 private:
  std::vector<ElementType> storage_;
};

template <class T>
class SparseTableT {
 public:
  typedef T ElementType;
  SparseTableT() : compare_() {}

  void Init(int hint_size) {}

  ElementType Inc(int id, ElementType count) {
    auto it = std::lower_bound(storage_.begin(), storage_.end(), id, compare_);
    if (it != storage_.end() && it->id == id) {
      return it->count += count;
    } else {
      IDCount target = {id, count};
      storage_.emplace(it, target);
      return count;
    }
  }

  ElementType Dec(int id, ElementType count) {
    auto it = std::lower_bound(storage_.begin(), storage_.end(), id, compare_);
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

  ElementType Count(int id) const {
    auto it = std::lower_bound(storage_.begin(), storage_.end(), id, compare_);
    if (it != storage_.end() && it->id == id) {
      assert(it->count > 0);
      return it->count;
    }
    return 0;
  }

  int NextNonZeroCountIndex(int index) const { return index; }
  int Size() const { return (int)storage_.size(); }
  int GetID(int index) const { return storage_[index].id; }
  ElementType GetCount(int index) const { return storage_[index].count; }

 private:
  struct IDCount {
    int id;
    ElementType count;
  };

  struct IDCountCompare {
    bool operator()(const IDCount& a, const IDCount& b) const {
      return a.id < b.id;
    }
    bool operator()(const IDCount& a, int b) const { return a.id < b; }
    bool operator()(int a, const IDCount& b) const { return a < b.id; }
  };

  IDCountCompare compare_;
  std::vector<IDCount> storage_;
};

template <class T>
class HashTableT {
 public:
  typedef T ElementType;
  HashTableT() : used_(0), deleted_(0) { storage_.resize(NextPrime(0)); }

  void Init(int hint_size) {}

  ElementType Inc(int id, ElementType count) {
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

  ElementType Dec(int id, ElementType count) {
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

  ElementType Count(int id) const {
    int pos = _FindIndex(id);
    const Item& item = storage_[pos];
    if (item.flag == kUsed) {
      assert(item.count > 0);
      return item.count;
    }
    return 0;
  }

  int NextNonZeroCountIndex(int index) const {
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

  int Size() const { return (int)storage_.size(); }

  int GetID(int index) const {
    assert(storage_[index].flag == kUsed);
    return storage_[index].id;
  }

  ElementType GetCount(int index) const {
    assert(storage_[index].flag == kUsed);
    return storage_[index].count;
  }

 private:
  static int NextPrime(int n) {
    static const int prime_list[] = {
        13,       23,       53,        97,        193,     389,     769,
        1543,     3079,     6151,      12289,     24593,   49157,   98317,
        196613,   393241,   786433,    1572869,   3145739, 6291469, 12582917,
        25165843, 50331653, 100663319, 201326611,
    };
    static const int prime_list_size =
        sizeof(prime_list) / sizeof(prime_list[0]);

    const int* first = prime_list;
    const int* last = prime_list + prime_list_size;
    const int* pos = std::lower_bound(first, last, n);
    return pos == last ? *(last - 1) : *pos;
  }

  enum {
    kUsed,
    kEmpty,
    kDeleted,
  };

  struct Item {
    int id;
    ElementType count;
    int flag;
    Item() : flag(kEmpty) {}
  };

  int used_;
  int deleted_;
  std::vector<Item> storage_;

 private:
  static int _FindIndex(int id, const std::vector<Item>& storage,
                        int storage_size) {
    int pos = static_cast<unsigned int>(id) % storage_size;
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
};

template <class T, template <class> class TableImpl>
class TableT : public TableImpl<T> {
 public:
  typedef T ElementType;
  typedef TableImpl<T> TableImplType;
  TableT() {}

 public:
  class const_iterator {
   private:
    const TableImplType* impl_;
    int index_;

   public:
    const_iterator(const TableImplType* impl, int index)
        : impl_(impl), index_(index) {}

    int id() const { return impl_->GetID(index_); }

    ElementType count() const { return impl_->GetCount(index_); }

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
    return const_iterator(this, this->NextNonZeroCountIndex(0));
  }

  const_iterator end() const { return const_iterator(this, this->Size()); }

  class __Proxy {
   private:
    TableImplType* impl_;
    int id_;

   public:
    __Proxy(TableImplType* impl, int id) : impl_(impl), id_(id) {}
    ElementType operator++() { return impl_->Inc(id_, 1); }
    ElementType operator+=(ElementType count) { return impl_->Inc(id_, count); }
    ElementType operator--() { return impl_->Dec(id_, 1); }
    ElementType operator-=(ElementType count) { return impl_->Dec(id_, count); }
    operator ElementType() { return impl_->Count(id_); }
  };

  __Proxy operator[](int id) { return __Proxy(this, id); }
  ElementType operator[](int id) const { return this->Count(id); }

  bool Save(const std::string& filename) const {
    std::ofstream ofs(filename.c_str());
    if (!ofs.is_open()) {
      ERROR("Failed to open \"%s\".", filename.c_str());
      return false;
    }

    auto first = begin();
    auto last = end();
    for (; first != last; ++first) {
      ofs << first.id() << ' ' << first.count() << std::endl;
    }
    return true;
  }

  bool Load(const std::string& filename) {
    std::ifstream ifs(filename.c_str());
    if (!ifs.is_open()) {
      ERROR("Failed to open \"%s\".", filename.c_str());
      return false;
    }

    int id;
    ElementType count;
    std::string line;
    while (std::getline(ifs, line)) {
      std::istringstream iss(line);
      if (iss >> id >> count) {
        (*this)[id] += count;
      }
    }
    return true;
  }
};

template <class Table>
class TablesT {
 public:
  typedef typename Table::ElementType ElementType;
  typedef Table TableType;
  TablesT() : d1_(0), d2_(0) {}

  void Init(int d1, int d2) {
    d1_ = d1;
    d2_ = d2;
    matrix_.clear();
    matrix_.resize(d1);
    for (int i = 0; i < d1_; i++) {
      matrix_[i].Init(d2);
    }
  }

  int d1() const { return d1_; }
  int d2() const { return d2_; }
  TableType& operator[](int i) { return matrix_[i]; }
  const TableType& operator[](int i) const { return matrix_[i]; }

  bool Save(const std::string& filename) const {
    std::ofstream ofs(filename.c_str());
    if (!ofs.is_open()) {
      ERROR("Failed to open \"%s\".", filename.c_str());
      return false;
    }

    for (int i = 0; i < d1_; i++) {
      const TableType& table = matrix_[i];
      auto first = table.begin();
      auto last = table.end();
      for (; first != last; ++first) {
        ofs << i << ' ' << first.id() << ' ' << first.count() << std::endl;
      }
    }
    return true;
  }

  bool Load(const std::string& filename) {
    std::ifstream ifs(filename.c_str());
    if (!ifs.is_open()) {
      ERROR("Failed to open \"%s\".", filename.c_str());
      return false;
    }

    int i, id;
    ElementType count;
    std::string line;
    while (std::getline(ifs, line)) {
      std::istringstream iss(line);
      if (iss >> i >> id >> count) {
        (*this)[i][id] += count;
      }
    }
    return true;
  }

 private:
  int d1_;
  int d2_;
  std::vector<TableType> matrix_;
};

typedef TableT<int, DenseTableT> DenseTable;
typedef TableT<int, SparseTableT> SparseTable;
typedef TableT<int, HashTableT> HashTable;
typedef TablesT<DenseTable> DenseTables;
typedef TablesT<SparseTable> SparseTables;
typedef TablesT<HashTable> HashTables;

#endif  // TABLE_H_
