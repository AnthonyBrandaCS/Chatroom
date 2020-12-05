#include "server.h"

bool hash_accept_test()
{
  const std::string test_key = "dGhlIHNhbXBsZSBub25jZQ==";
  const std::string acceptance = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";

  std::string test_result = getSockAccept(test_key);

  return test_result == acceptance;
}

bool extract_key_test()
{
  const std::string expected_key = "8QmTpd/hJUMox0NSYsV01Q==";

  const std::string test_request =
      "GET /chat HTTP/1.1\n"
      "Host: localhost:8000\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:82.0) Gecko/20100101 Firefox/82.0\n"
      "Accept: */*\n"
      "Accept-Language: en-US,en;q=0.5\n"
      "Accept-Encoding: gzip, deflate\n"
      "Sec-WebSocket-Version: 13\n"
      "Origin: null\n"
      "Sec-WebSocket-Extensions: permessage-deflate\n"
      "Sec-WebSocket-Key: 8QmTpd/hJUMox0NSYsV01Q==\n"
      "Connection: keep-alive, Upgrade\n"
      "Pragma: no-cache\n"
      "Cache-Control: no-cache\n"
      "Upgrade: websocket\n";

  std::string test_key = getSockKey(test_request);

  return test_key == expected_key;
}

bool data_decode_test()
{

  /*
  F RRR OPCODE MASK LENGTH           MASKING KEY                  0         1        2       3        4        5        6       7         8         9       10       11
  1 000 0001   1    0001100 11010010001011001001111111000000   10011010 01001001 11110011 10101100 10111101 00001100 11001000 10101111 10100000 01000000 11111011 11100001

  10000001 10001100 11010010 00101100 10011111 11000000   -> message  10011010 01001001 11110011 10101100 10111101 00001100 11001000 10101111 10100000 01000000 11111011 11100001
  */
  const char test_data[1024] =
      {
          (char)0b10000001, (char)0b10001100, (char)0b11010010, (char)0b00101100,
          (char)0b10011111, (char)0b11000000, (char)0b10011010, (char)0b01001001,
          (char)0b11110011, (char)0b10101100, (char)0b10111101, (char)0b00001100,
          (char)0b11001000, (char)0b10101111, (char)0b10100000, (char)0b01000000,
          (char)0b11111011, (char)0b11100001};

  const size_t test_length = 18;
  const std::string expected_message = "Hello World!";

  std::string message = DecodeWebSocket(test_data, test_length);

  return message == expected_message;
}

bool unit_test()
{
  std::vector<bool> test_passes;
  std::vector<std::string> test_names;

  test_passes.push_back(hash_accept_test());
  test_names.push_back("hash_accept_test");

  test_passes.push_back(extract_key_test());
  test_names.push_back("extract_key_test");

  test_passes.push_back(data_decode_test());
  test_names.push_back("data_decode_test");

  bool pass = true;
  for (int i = 0; i < test_passes.size(); i++)
  {
    std::cout << test_names[i] << " : " << (test_passes[i] ? "Passed" : "Failed") << std::endl;
    if (!test_passes[i])
    {
      pass = false;
    }
  }
  return pass;
}