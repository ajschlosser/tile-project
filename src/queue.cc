#include "queue.h"
#include "objects.h"
#include <map>
#include <queue>

std::mutex e_mtx ;

template <typename T> std::tuple<int, std::shared_ptr<events::eventData<T>>> events::pop()
{
  if (_q<T>.size() > 0)
  {
    auto [id, data] = _q<T>.front();
    e_mtx.lock();
    _q<T>.pop();
    e_mtx.unlock();
    return std::make_tuple(id, data);
  }
  return std::make_tuple(NULL, nullptr);
}

// std::queue<std::map<int, queue::eventData<MobObject>>> mobV;

// template <typename T> void queue::push(int e, eventData<T> d)
// {
//   // std::map<std::string, std::shared_ptr<MobObject>> e = {}
//   if (d->trigger->get()->type == tileObject::MOB)
//     mobV.push({ e, d });
// }

// template <typename T> std::shared_ptr<queue::eventData<T>> queue::pop(int t)
// {
//   if (t == tileObject::MOB)
//     if (mobV.size() > 0)
//     {
//       auto i = std::make_shared<queue::eventData<MobObject>>(mobV.front());
//       mobV.pop();
//       return i;
//     }
// }