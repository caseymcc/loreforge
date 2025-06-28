Table 4.1: Top 15 General-Purpose Open LLMs: Benchmark Performance and Key Characteristics
| Rank | Model Name | Developer | Parameters | MMLU Score (%) | GPQA Score (%) | MATH Score (%) | Key Strengths/Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | Nemotron Ultra 253B | Nvidia | 253B | 76.0 | 57.1 | 80.1 | Exceptional math and reasoning capabilities. |   
| 2 | DeepSeek-R1 | DeepSeek AI | 671B (MoE) | 71.5 | 53.6 | 79.8 | State-of-the-art reasoning model, excels in math and agentic tasks. |   
| 3 | Llama 3.1 405b | Meta | 405B | - | 51.1 | 73.8 | Massive dense model with strong tool use and general knowledge. |   
| 4 | Llama 4 Maverick | Meta | - | 69.8 | 53.6 | - | Very large context window (10M tokens), strong adaptive reasoning. |   
| 5 | Qwen/Qwen2.5-72B-Instruct | Alibaba | 72B | 84.2 | - | - | Top-tier performance on the (older) Hugging Face leaderboard average. |   
| 6 | DeepSeek V3 0324 | DeepSeek AI | 671B (MoE) | 64.8 | 38.8 | 59.4 | Strong reasoning and coding, more efficient than R1. |   
| 7 | Llama 3.3 70B | Meta | 70B | - | 46.7 | 68.0 | Excellent tool use and high throughput, very balanced. |   
| 8 | MaziyarPanahi/calme-3.2-instruct-78b | Community | 78B | 70.0 (MMLU-PRO) | 20.4 | 40.3 | Top-ranked fine-tuned model on the Hugging Face leaderboard. |   
| 9 | Qwen2.5-VL-32B | Alibaba | 32B | - | 42.9 | - | Strong multimodal model with good reasoning and tool use. |   
| 10 | Phi 4 Reasoning Plus | Microsoft | 14B | 76.0 | - | - | High-performing small model with a focus on reasoning. |   
| 11 | Gemma 2 27B Instruct | Google | 27B | 76.2 | 59.1 (BFCL) | - | Very strong for its size, leads in Chatbot Arena Elo for open models. |   
| 12 | QwQ-32B | Alibaba | 32.5B | 65.2 | - | - | Specialized reasoning model from Alibaba, performs well on complex tasks. |   
| 13 | Yi 1.5 34B Chat | 01 AI | 34B | 76.8 | - | - | Strong competitor in the ~30B parameter class. |   
| 14 | Mixtral 8x22b Instruct-v0.1 | Mistral AI | 141B (MoE) | 77.8 | - | - | Efficient MoE model with strong general capabilities. |   
| 15 | Phi-3-medium-4k-instruct | Microsoft | 14B | 78.0 | - | - | Highly capable SLM that competes with much larger models. |   


Table 5.1: Top 10 Open-Source Coding LLMs: Performance on Key Benchmarks
| Rank | Model Name | Developer | Parameters | HumanEval (pass@1, %) | MBPP (Accuracy, %) | SWE-Bench (% Resolved) | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | DeepSeek-R1 | DeepSeek AI | 671B (MoE) | ~37.0* | - | 49.2 | Top-tier reasoning and agentic coding; also excels on LiveCodeBench (64.3%). |   
| 2 | DeepSeek V3 0324 | DeepSeek AI | 671B (MoE) | - | - | 38.8 | Strong successor to R1, optimized for efficiency. |   
| 3 | Qwen2.5-Coder-32B-Instruct | Alibaba | 32B | 92.7 | 90.2 | ~31.0* | Exceptional HumanEval/MBPP scores, indicating strong Python proficiency. |   
| 4 | DeepSeek-Coder-V2-Instruct | DeepSeek AI | 236B (MoE) | 85.4 | 89.4 | - | Very strong performance from a specialized MoE coder model. |   
| 5 | Qwen2.5-Coder-14B-Instruct | Alibaba | 14B | 89.6 | 86.2 | ~25.4* | Excellent balance of performance and size, outperforming many larger models. |   
| 6 | Llama 4 Maverick | Meta | - | ~62.0 | ~78.0 | - | Massive context potential for whole-codebase tasks, decent function-level performance. |   
| 7 | DeepSeek-Coder-V2-Lite-Instruct | DeepSeek AI | 16B (MoE) | 81.1 | 82.8 | - | Highly efficient MoE coder, great for local use. |   
| 8 | CodeQwen1.5-7B-Chat | Alibaba | 7B | 83.5 | 70.6 | - | Very strong 7B model, beats many larger models on HumanEval. |   
| 9 | Qwen2.5-Coder-7B-Instruct | Alibaba | 7B | 88.4 | 83.5 | ~20.3* | Outstanding performance for a 7B model, leading its class. |   
| 10 | DeepSeek-Coder-33B-Instruct | DeepSeek AI | 33B | 79.3 | 70.0 | - | A powerful dense coder model from DeepSeek. |   


Table 6.1: Estimated VRAM Requirements for Model Weights at Various Quantization Levels
| Model Name | Parameters | FP16 (16-bit) VRAM (GB) | Q8_0 (8-bit) VRAM (GB) | Q5_K_M (~5-bit) VRAM (GB) | Q4_K_M (~4.5-bit) VRAM (GB) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Llama 3 / Qwen 2 / Mistral 7B | 7B | ~14.0 | ~7.7 | ~5.1 | ~4.4 |
| Llama 3 8B | 8B | ~16.0 | ~9.0* | ~5.8 | ~5.0 |
| Phi-3 Medium | 14B | ~28.0 | ~14.0 | ~9.2 | ~7.9 |
| DeepSeek-Coder 33B | 33B | ~66.0 | ~33.0 | ~21.7 | ~18.6 |
| Llama 3 / Qwen 2 70B | 70B | ~140.0 | ~75.0* | ~49.2 | ~42.1 |
Note: VRAM usage is calculated based on parameter count and bits-per-weight, and supplemented with data from community tests and model cards. Actual usage will be slightly higher due to overhead. 8-bit Llama 3 8B requires ~9 GB, and 8-bit 70B models can exceed 70 GB.   

