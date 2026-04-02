# Async Logging Is Not a Silver Bullet
### Formatting, Output, and What Actually Limits Throughput

Async logging is often presented as an obvious optimization:

- synchronous logging is slow  
- async logging is fast  
- therefore async logging is better  

In practice, this is an oversimplification.

Async logging does not remove the cost of logging.  
It only redistributes it — across threads, queues, memory, and time.

To understand its real behavior, we need to separate what async logging actually consists of.

---

# 1. Async Logging = Two Independent Problems

Async logging is not a single mechanism. It is a combination of two distinct subsystems:

- **Async formatting**
- **Async output**

These are often conflated, but they have very different constraints and trade-offs.

---

## Async Formatting

This includes:

- capturing arguments  
- copying or serializing data  
- optionally formatting later (deferred formatting)  

The key question is:

> **Can the backend safely reconstruct the data later?**

If not, formatting cannot be deferred.

---

## Async Output

This includes:

- queues  
- backend worker thread(s)  
- sinks (file, console, network)  
- flushing and fsync  

This part determines throughput and I/O behavior.

---

# 2. Deferred Formatting Is Conditional, Not Universal

Deferred formatting sounds attractive:

> enqueue format string + arguments → format later on a backend thread

But this only works if arguments remain valid.

---

## When It Breaks

```cpp
std::string s = MakeText();
std::string_view sv = s;
LOG_INFO("{}", sv);
s.clear();
```

If only the `string_view` is stored, the data becomes invalid before the backend processes it.

The same problem appears with:

- `char*` to temporary buffers  
- references to mutable objects  
- objects with internal pointers  
- container views  
- externally owned data  

This is not a corner case — it is a fundamental limitation.

---

# 3. How Systems Actually Solve It

There are only a few real solutions.

---

## 1. Copy the Data (Safe Capture)

The logging system copies data at the call site:

- `string_view` → copied into owned buffer  
- `char*` → copied  
- objects → copied or serialized  

This guarantees correctness.

But it also means:

> **Deferred formatting requires eager data capture — i.e. copying.**

---

## 2. Format Immediately

If copying is unsafe or too complex:

```cpp
LOG_INFO("{}", complex_object);
```

The system formats immediately on the producer thread and stores the resulting string.

This avoids lifetime issues but increases hot-path latency.

---

## 3. Custom Serialization

For complex types:

- user-defined serialization  
- custom codecs  
- explicit control over what is captured  

This is flexible — but increases complexity.

---

# 4. The Hidden Cost of Deferred Formatting

At this point, the pipeline is not:

```
enqueue → format → output
```

It is:

```
capture (copy/serialize) → enqueue → format → output
```

So the real trade-off becomes:

- replacing formatting cost with copying + serialization  
- introducing memory overhead  
- adding complexity  

In many cases:

> **copy + deferred format ≈ immediate format**

---

# 5. Async Output Does Not Increase Throughput

Async logging decouples the caller from I/O.  
It does not make I/O faster.

---

## The Bottleneck Still Exists

If your sinks process:

```
100k messages/sec
```

and producers generate:

```
1M messages/sec
```

then:

```
900k messages/sec accumulate
```

This is inevitable.

---

## The Queue Is Not Magic

A queue does not remove work.

It only delays when you notice that the work cannot be completed.

> **The queue is not acceleration — it is debt storage.**

---

# 6. Single Backend Thread: A Hard Limit

Many async logging systems use:

- multiple producers  
- a **single backend thread**

---

## No Automatic Scaling

Even with many producer threads:

- there is still one consumer  
- throughput is limited by that thread  

Async logging does not scale across CPU cores automatically.

---

## Multiple Sinks Multiply Cost

For each log message:

```
format + write file A + write file B + write console
```

Total cost = sum of all sinks.

---

### Critical Consequence

> **Adding a slow sink slows down all logging.**

---

# 7. What Happens Under Sustained Overload

When:

```
producer rate > backend rate
```

the system must choose how to behave.

---

## Possible Strategies

### 1. Grow Buffers
### 2. Block Producers
### 3. Drop Messages
### 4. Overwrite Old Data

---

## Key Insight

> **Async logging systems do not eliminate overload — they define how overload is handled.**

---

# 8. Why Deferred Formatting Is Often Overrated

Deferred formatting moves work from producers to backend.

It does not eliminate it.

---

## Real Benefit

> **lower latency on the hot path**

---

## Key Insight

> **The benefit of deferred formatting is not that it is cheaper — it is that it moves the cost away from the caller.**

---

# 9. Practical Design Convergence

Many systems converge to:

> **synchronous formatting + asynchronous output**

---

# 10. Final Thought

Async logging is not about making logging free.

> **It is about choosing where and how you pay for it.**

