// WASI semaphore stubs for OpenH264
// OpenH264 has some threading code that we can't completely eliminate
// In single-threaded mode, semaphores should just be no-ops since there's no actual waiting

#include <stdint.h>
#include <iostream>

#ifdef __wasi__

// Semaphore type - just track a value
typedef struct {
    int value;
} sem_t;

// Single-threaded semaphore implementations
// In single-threaded mode, sem_wait/post are effectively no-ops
// since there's nothing to wait for or signal

extern "C" {
    int sem_init(sem_t* sem, int pshared, unsigned int value) {
        if (!sem) return -1;
        sem->value = value;
        std::cout << "[WASI sem_init] value=" << value << std::endl;
        return 0;
    }
    
    int sem_destroy(sem_t* sem) {
        if (!sem) return -1;
        std::cout << "[WASI sem_destroy]" << std::endl;
        return 0;
    }
    
    int sem_wait(sem_t* sem) {
        if (!sem) return -1;
        // In single-threaded mode, if value is 0, we'd deadlock
        // This should never happen in proper single-threaded use
        if (sem->value <= 0) {
            std::cout << "[WASI sem_wait] WARNING: would block (value=" << sem->value << ") - likely threading issue!" << std::endl;
            // Don't actually block in WASI - just proceed
            // This might cause issues but prevents hanging
        } else {
            sem->value--;
        }
        std::cout << "[WASI sem_wait] value now=" << sem->value << std::endl;
        return 0;
    }
    
    int sem_trywait(sem_t* sem) {
        if (!sem) return -1;
        if (sem->value > 0) {
            sem->value--;
            std::cout << "[WASI sem_trywait] success, value now=" << sem->value << std::endl;
            return 0;
        }
        std::cout << "[WASI sem_trywait] would block" << std::endl;
        return -1;  // Would block
    }
    
    int sem_timedwait(sem_t* sem, const void* abs_timeout) {
        // Same as sem_wait in single-threaded mode
        return sem_wait(sem);
    }
    
    int sem_post(sem_t* sem) {
        if (!sem) return -1;
        sem->value++;
        std::cout << "[WASI sem_post] value now=" << sem->value << std::endl;
        return 0;
    }
    
    int sem_getvalue(sem_t* sem, int* sval) {
        if (!sem || !sval) return -1;
        *sval = sem->value;
        return 0;
    }
}

#endif // __wasi__
