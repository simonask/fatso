#include <yaml.h>
#include <stdio.h>

#if !defined(FATSO_TEST)
#error "Fatso didn't add define FATSO_TEST from base configuration in fatso.yml"
#endif

int main(int argc, char const *argv[])
{
  yaml_parser_t yaml;
  yaml_parser_initialize(&yaml);
  printf("Hello, World!\n");
  yaml_parser_delete(&yaml);
  return 0;
}
