#!/usr/bin/env python3
"""Generate a synthetic Rime shared-data directory for deployment benchmarks.

Produces a deterministic set of schemas and dictionaries so that deployments
are reproducible across runs and across old/new builds.

Usage:
  gen_data.py --out DIR [options]

Options:
  --schemas N       number of independent schemas to generate (default 6)
  --entries M       dict entries per schema (default 150000, ~4.5MB each)
  --shared-dict N   instead: create N schemas that all share one dictionary
                    but with distinct prisms (exercises the serial fallback)
  --seed S          random seed (default 42)
  --overwrite       regenerate even if --out already exists
"""

import argparse
import os
import random

PINYIN_SYLLABLES = [
    'a', 'ai', 'an', 'ang', 'ao', 'ba', 'bai', 'ban', 'bang', 'bao', 'bei',
    'ben', 'bi', 'bian', 'biao', 'bie', 'bin', 'bing', 'bo', 'bu', 'ca',
    'cai', 'can', 'cang', 'cao', 'ce', 'cen', 'ceng', 'cha', 'chai', 'chan',
    'chang', 'chao', 'che', 'chen', 'cheng', 'chi', 'chong', 'chou', 'chu',
    'chua', 'chuai', 'chuan', 'chuang', 'chui', 'chun', 'chuo', 'ci', 'cong',
    'cou', 'cu', 'cuan', 'cui', 'cun', 'cuo', 'da', 'dai', 'dan', 'dang',
    'dao', 'de', 'deng', 'di', 'dian', 'diao', 'die', 'ding', 'diu', 'dong',
    'dou', 'du', 'duan', 'dui', 'dun', 'duo', 'e', 'en', 'er', 'fa', 'fan',
    'fang', 'fei', 'fen', 'feng', 'fo', 'fou', 'fu', 'ga', 'gai', 'gan',
    'gang', 'gao', 'ge', 'gei', 'gen', 'geng', 'gong', 'gou', 'gu', 'gua',
    'guai', 'guan', 'guang', 'gui', 'gun', 'guo', 'ha', 'hai', 'han', 'hang',
    'hao', 'he', 'hei', 'hen', 'heng', 'hong', 'hou', 'hu', 'hua', 'huai',
    'huan', 'huang', 'hui', 'hun', 'huo', 'ji', 'jia', 'jian', 'jiang',
    'jiao', 'jie', 'jin', 'jing', 'jiong', 'jiu', 'ju', 'juan', 'jue',
    'jun', 'ka', 'kai', 'kan', 'kang', 'kao', 'ke', 'ken', 'keng', 'kong',
    'kou', 'ku', 'kua', 'kuai', 'kuan', 'kuang', 'kui', 'kun', 'kuo', 'la',
    'lai', 'lan', 'lang', 'lao', 'le', 'lei', 'leng', 'li', 'lia', 'lian',
    'liang', 'liao', 'lie', 'lin', 'ling', 'liu', 'long', 'lou', 'lu',
    'luan', 'lue', 'lun', 'luo', 'ma', 'mai', 'man', 'mang', 'mao', 'me',
    'mei', 'men', 'meng', 'mi', 'mian', 'miao', 'mie', 'min', 'ming', 'miu',
    'mo', 'mou', 'mu', 'na', 'nai', 'nan', 'nang', 'nao', 'ne', 'nei',
    'nen', 'neng', 'ni', 'nian', 'niang', 'niao', 'nie', 'nin', 'ning',
    'niu', 'nong', 'nu', 'nuan', 'nue', 'nuo', 'o', 'ou', 'pa', 'pai',
    'pan', 'pang', 'pao', 'pei', 'pen', 'peng', 'pi', 'pian', 'piao', 'pie',
    'pin', 'ping', 'po', 'pou', 'pu', 'qi', 'qia', 'qian', 'qiang', 'qiao',
    'qie', 'qin', 'qing', 'qiong', 'qiu', 'qu', 'quan', 'que', 'qun',
    'ran', 'rang', 'rao', 're', 'ren', 'reng', 'ri', 'rong', 'rou', 'ru',
    'ruan', 'rui', 'run', 'ruo', 'sa', 'sai', 'san', 'sang', 'sao', 'se',
    'sen', 'seng', 'sha', 'shai', 'shan', 'shang', 'shao', 'she', 'shei',
    'shen', 'sheng', 'shi', 'shou', 'shu', 'shua', 'shuai', 'shuan',
    'shuang', 'shui', 'shun', 'shuo', 'si', 'song', 'sou', 'su', 'suan',
    'sui', 'sun', 'suo', 'ta', 'tai', 'tan', 'tang', 'tao', 'te', 'teng',
    'ti', 'tian', 'tiao', 'tie', 'ting', 'tong', 'tou', 'tu', 'tuan', 'tui',
    'tun', 'tuo', 'wa', 'wai', 'wan', 'wang', 'wei', 'wen', 'weng', 'wo',
    'wu', 'xi', 'xia', 'xian', 'xiang', 'xiao', 'xie', 'xin', 'xing',
    'xiong', 'xiu', 'xu', 'xuan', 'xue', 'xun', 'ya', 'yan', 'yang', 'yao',
    'ye', 'yi', 'yin', 'ying', 'yo', 'yong', 'you', 'yu', 'yuan', 'yue',
    'yun', 'za', 'zai', 'zan', 'zang', 'zao', 'ze', 'zei', 'zen', 'zeng',
    'zha', 'zhai', 'zhan', 'zhang', 'zhao', 'zhe', 'zhei', 'zhen', 'zheng',
    'zhi', 'zhong', 'zhou', 'zhu', 'zhua', 'zhuai', 'zhuan', 'zhuang',
    'zhui', 'zhun', 'zhuo', 'zi', 'zong', 'zou', 'zu', 'zuan', 'zui',
    'zun', 'zuo',
]


def rand_code(rng, syl):
    return ''.join(rng.choice(syl) for _ in range(rng.randint(1, 3)))


def rand_text(rng):
    return ''.join(chr(0x4E00 + rng.randint(0, 4000))
                   for _ in range(rng.randint(1, 4)))


def gen_dict(rng, path, name, entries):
    with open(path, 'w') as f:
        f.write('---\nname: %s\nversion: "1.0"\nsort: by_weight\n'
                'columns:\n  - text\n  - code\n  - weight\n...\n' % name)
        seen = set()
        weight = 100000.0
        for _ in range(entries):
            while True:
                text = rand_text(rng)
                if text not in seen:
                    seen.add(text)
                    break
            f.write('%s\t%s\t%.4f\n' % (text, rand_code(rng, PINYIN_SYLLABLES),
                                        weight))
            weight -= rng.uniform(0.0, 0.1)


def gen_schema(out_dir, schema_id, dict_name=None, prism=None):
    body = 'schema:\n  schema_id: %s\n  name: %s\n' % (schema_id, schema_id)
    if dict_name:
        body += 'translator:\n  dictionary: %s\n' % dict_name
        if prism:
            body += '  prism: %s\n' % prism
    with open(os.path.join(out_dir, '%s.schema.yaml' % schema_id), 'w') as f:
        f.write(body)


def write_default_yaml(out_dir, schema_list):
    lines = [
        'config_version: "0.1"',
        'schema_list:',
    ]
    for sid in schema_list:
        lines.append('  - schema: %s' % sid)
    lines.append('')
    with open(os.path.join(out_dir, 'default.yaml'), 'w') as f:
        f.write('\n'.join(lines))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--schemas', type=int, default=6)
    ap.add_argument('--entries', type=int, default=150000)
    ap.add_argument('--shared-dict', type=int, default=0,
                    help='create this many schemas sharing one dictionary '
                         'with distinct prisms (serial fallback path)')
    ap.add_argument('--seed', type=int, default=42)
    ap.add_argument('--overwrite', action='store_true')
    args = ap.parse_args()

    if os.path.exists(args.out):
        if not args.overwrite:
            print('error: %s exists; use --overwrite' % args.out)
            raise SystemExit(1)
        import shutil
        shutil.rmtree(args.out)

    os.makedirs(args.out)
    rng = random.Random(args.seed)
    schema_list = []

    n = args.shared_dict if args.shared_dict else args.schemas
    for i in range(1, n + 1):
        if args.shared_dict:
            dict_name = 'shared'
            schema_id = 'sh%d' % i
            prism = 'shared_prism%d' % i
        else:
            dict_name = schema_id = 's%d' % i
            prism = None
        if i == 1 or not args.shared_dict:
            gen_dict(rng, os.path.join(args.out, '%s.dict.yaml' % dict_name),
                     dict_name, args.entries)
        gen_schema(args.out, schema_id, dict_name, prism)
        schema_list.append(schema_id)

    write_default_yaml(args.out, schema_list)
    print('generated %d schema(s) in %s' % (len(schema_list), args.out))


if __name__ == '__main__':
    main()
