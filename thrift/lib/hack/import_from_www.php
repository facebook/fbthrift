<?hh
/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class ImportThriftLibFromWWW {
  public static function run(array<string> $argv): void {
    invariant(count($argv) >= 2, 'Need to specify path to www');
    self::importHackLib($argv[1]);
  }

  protected static function importHackLib(string $www_path): void {
    self::walkPHPFiles(
      $www_path.'/flib/thrift/core',
      array(''),
      function (string $root, string $path, string $file) {
        if (strpos($path, '__tests__') !== false) {
          echo "Skipping $path/$file\n";
          return;
        }

        $contents = file_get_contents($root.$path.'/'.$file);
        $contents = preg_replace(
          '#/\* BEGIN_STRIP \*/.*?/\* END_STRIP \*/#s',
          '',
          $contents,
        );
        $contents = preg_replace(
          '#/\* INSERT (.*?) END_INSERT \*/#s',
          '\\1',
          $contents,
        );

        $license = array(
          '/**',
          '* Copyright (c) 2006- Facebook',
          '* Distributed under the Thrift Software License',
          '*',
          '* See accompanying file LICENSE or visit the Thrift site at:',
          '* http://developers.facebook.com/thrift/',
          '*',
          '* @package '.str_replace('/', '.',dirname('thrift'.$path.'/'.$file)),
          '*/',
        );
        $replacement = "\\1\n".implode("\n", $license)."\n";
        $contents = preg_replace(
          '#^(<\?hh(?: +// +[a-z]+)?\n)//.*\n#',
          $replacement,
          $contents,
        );

        if (!file_exists(__DIR__.'/src/'.$path)) {
          mkdir(__DIR__.'/src/'.$path, 0755, true);
        }
        file_put_contents(__DIR__.'/src/'.$path.'/'.$file, $contents);
        echo "Imported $path/$file\n";
      }
    );
  }

  protected static function walkPHPFiles(
    string $root,
    array<string> $paths,
    (function (string, string, string): void) $cb,
  ): void {
    for ($i = 0; $i < count($paths); $i++) {
      $path = $paths[$i];
      $full_path = $root.$path;

      $files = scandir($full_path);
      foreach ($files as $file) {
        if ($file === '.' || $file === '..') {
          continue;
        }

        if (is_dir($full_path.'/'.$file)) {
          $paths[] = $path.'/'.$file;
        }

        if (!preg_match('/\.php$/', $file)) {
          continue;
        }

        $cb($root, $path, $file);
      }
    }
  }
}

ImportThriftLibFromWWW::run($argv);
