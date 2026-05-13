# Multithreaded Producer-Consumer Simulation

This project is a multithreaded Producer-Consumer simulation developed in C for the Operating Systems course. It uses POSIX threads (pthreads), mutexes, and semaphores to ensure thread-safe operations across shared bounded buffers. It also features a dynamic configuration parser and an independent Monitor Thread to detect system deadlocks and starvation.

- **Course:** Operating Systems — Spring 2025-2026
- **University:** Sakarya University
- **Instructor:** Prof. Dr. Ünal Çavuşoğlu

## Team
- Vildan Karaca — B231202027
- Ayşenur Yılmaz — B231202019
- Rana İrem Özen — B241202002
- Elif Gül Arslan — B231202061

## 🛠️ Prerequisites
* Linux environment (e.g., Ubuntu or WSL)
* GCC Compiler
* Make utility

## ✨ Features
* Dynamic configuration via text files
* Multiple producer/consumer threads (POSIX pthreads)
* Mutex + semaphore synchronization
* Real-time deadlock detection via Monitor Thread

## 🚀 How to Compile and Run
This project includes a `Makefile` to automate the build process and seamlessly run different experimental setups.

### 1. Custom Sandbox Testing (Default)
If you want to test the system with random inputs or create your own custom setup, simply edit the `configs/config.txt` file and run: 
```bash
make
```
> **Note:** This command automatically compiles the code and executes the program using the default config file.

### 2. Execute the Program (Manual Execution)
If the project is already compiled and you want to run the executable manually with a specific configuration file, you can use:
```bash
./os_project configs/config.txt
```

### 3. 🧪 Reproducing The Experiments
To reproduce the exact performance metrics and deadlock scenarios presented in the project report, use the following shortcuts. The Makefile will automatically load and execute the correct setup from the configs/ folder:
**Experiment 1 (Low Load):** 
```bash
make exp1
```
**Experiment 2 (High Load):**
```bash
make exp2
```
**Experiment 3 (Deadlock):**
```bash
make exp3
```
**Experiment 4 (Bottleneck):**
```bash
make exp4
```
**Experiment 5 (Circular Dependency):**
```bash
make exp5
```

### 4. Clean Build Files
To remove the generated object files and the executable, use:
```bash
make clean
```

## ⚙️ Configuration (config.txt)
The system dynamically reads its setup from the text files located in the `configs/` directory. If you want to try new things, open `configs/config.txt`. You can modify buffer sizes, thread counts, routing paths (e.g., A>C1>B), and execution intervals without recompiling the code.

**Note on Buffer Allocation:** The system features implicit memory handling. If you do not define a size for a buffer (e.g., omitting B[n]), the system will automatically allocate a safe, size-1 dummy buffer in the background to prevent memory violations (Segmentation Faults). You only need to declare the buffers you actively use!

### Syntax Reference
```text
A[n]    // Buffer A with size n
B[n]
t:60    //defines runtime. app will exit after 60 sec
P1>A    //producer with id P1 produces component A
P2>B    //producer with id P3 produces component B
A>C1    //consumer with id C1 consumes component A
B>C2
B>C3>A  //consumer with id C3 consumes component B 
        //and produces component A 
P1:2    //Producer with id P1 will create component each 2ms
P2:3      
C1:2    //Consumer with id C1 will consume component each 2ms 
C2:3
C3:2
```

## 📂 Project Structure
```text
📁 B231202027/
├── configs/
│   ├── config.txt           
│   ├── exp1_LowLoad.txt     
│   ├── exp2_HighLoad.txt
│   ├── exp3_Deadlock.txt
│   ├── exp4_Bottleneck.txt
│   └── exp5_CircularDependency.txt
├── Makefile
├── README.md
├── src/
│   ├── consumer.c
│   ├── producer.c
│   ├── main.c
│   └── common/
│       ├── utils.c
│       └── utils.h
├── docs/
│   └── OS-Report.pdf
└── media/
    └── experiments_with_explanations.mp4
```    