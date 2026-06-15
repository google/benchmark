#if defined(__clang__)
#define THREAD_ANNOTATION_ATTRIBUTE_(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE_(x)
#endif

#define CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE_(capability(x))
#define ACQUIRE() THREAD_ANNOTATION_ATTRIBUTE_(acquire_capability())
#define RELEASE() THREAD_ANNOTATION_ATTRIBUTE_(release_capability())

class CAPABILITY("mutex") Mutex {
 public:
  void lock() ACQUIRE();
  void unlock() RELEASE();
};

void Mutex::lock() ACQUIRE() {}
void Mutex::unlock() RELEASE() {}

int main() {
  Mutex m;
  m.lock();
  m.unlock();
  return 0;
}
