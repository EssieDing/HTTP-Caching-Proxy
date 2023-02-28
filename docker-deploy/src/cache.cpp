#include "cache.h"

Response Cache::get(string request_line) {
  cout << "get cache: " << request_line << endl; 
  cout << "cache size: " << cache_map.size() << endl;
    mtx.lock();
    unordered_map<std::string, Response>::iterator it = cache_map.begin();
    it = cache_map.find(request_line);
    if (cache_map.count(request_line) == 0) {
        mtx.unlock();
        return Response(); // create new response and return
    }
    mtx.unlock();
    return it->second;
};

void Cache::put(string & request_line, Response response){
  cout << "put into cache: " << response.response_line << endl;
  cout << "cache size: " << cache_map.size() << endl;
      mtx.lock();
      if (cache_map.size() == size && cache_map.count(request_line) == 0){
        cache_map.erase (cache_map.begin());
        // printLog(-1, ": NOTE " + cache_map.begin()->first + " removed from cache")
      }
      cache_map.insert(pair<string, Response>(request_line, response));
      // printLog
      mtx.unlock();
    };

void Cache::remove(string & request_line){
      cout << "remove from cache" << request_line << endl;
      mtx.lock();
      cache_map.erase(request_line);
      // printLog(-1, ": NOTE " + request_line + " removed from cache")
      mtx.unlock();
    };