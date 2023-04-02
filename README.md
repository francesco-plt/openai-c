# openai-c

Since everyone is wrapping chatGPT APIs, I decided to do the same. In C.
Actually this is just a small chatbot which takes keyboard input from the terminal and sends it to `gpt-3.5-turbo`, which will then respond with a message.

## Usage

```shell
$ git clone https://github.com/francesco-plt/openai-c
$ cd openai-c
$ export OPENAI_API_KEY="sk-xxx"
$ make build && make run
```
