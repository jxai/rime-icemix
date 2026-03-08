# 雾凇混拼

基于雾凇拼音的混拼输入方案

## 功能概要

- **混拼输入**
  - **全拼 | 双拼 | 简拼** 混合输入，方便随心
  - 尤其适合在从全拼向双拼输入过渡的用户
  - 长词组输入，简拼更快捷
  - 支持主流双拼方案：小鹤、智能ABC、拼音加加、微软、搜狗、紫光、自然码
- **反查增强**
  - 部件拆字反查（`uU` + 部件拼音）同样支持混拼，全、双拼皆可输入
  - 反查字带音调显示，例如：**顼 xū**
- **词库优化**
  - 附带[白霜拼音](https://github.com/gaboolic/rime-frost)优化的中文词库
  - 默认仍使用雾凇内建词库，可在安装时选择白霜词库，或通过配置文件切换
- **界面改进**
  - 预编辑区用 **⸢**…**⸣** 标记当前候选对应的输入片段，清晰明了
  - 更改Emoji关闭状态图标为🈚️，直观易懂
- **安装方便**
  - 通过[东风破](https://github.com/rime/plum)一步安装配置，开箱即用

> [!NOTE]
> 关于雾凇拼音的基本特性和使用方式，参考[官方网站](https://dvel.me/posts/rime-ice/)和[GitHub代码仓库](https://github.com/iDvel/rime-ice)。

## 安装

### 通过[东风破 (plum)](https://github.com/rime/plum)安装 (推荐)

```bash
bash plum/rime-install jxai/rime-icemix:recipes/icemix[:double=<double>,simp=<simp>,dict=<dict>]
```

`<double>`选择双拼方案，可选参数如下：

- `flypy` (默认): 小鹤
- `abc`: 智能ABC
- `jiajia`: 拼音加加
- `mspy`: 微软
- `sogou`: 搜狗
- `ziguang`: 紫光
- `zrm`: 自然码

`<double>`对应的混拼方案会自动加到`default.custom.yaml`的`schema_list`中，重新部署后即可使用。小鹤双拼对应方案显示名为“凇鹤混拼”，其他方案类似。字根反查和中英混输使用的双拼方案也会相应地自动配置。

`<simp>`设置简拼模式，可选参数如下：

- `normal` (默认): 普通模式，简拼作为`abbrev`处理，优先级较低，适合输入长词组
- `disable`: 禁止简拼输入
- `boost`: 提高简拼优先级，但会影响正常全、双拼候选的优先排序，慎用

`<dict>`指定中文词库，可选参数如下：

- `ice` (默认): 雾凇内建词库
- `frost`: [白霜拼音](https://github.com/gaboolic/rime-frost)优化词库

> [!NOTE]
> 由于东风破安装脚本`rime-install`的局限，再次安装新方案时并不会删除之前选择的方案配置。一般不影响使用，如有必要可手工编辑对应的`.custom.yaml`进行清理。

### 手动安装

使用[Git](https://github.com/jxai/rime-icemix.git)或[手动下载](https://github.com/jxai/rime-icemix/archive/refs/heads/main.zip)仓库文件到平台相应的Rime配置目录。通过补丁配置如下文件，其中相关参数如前所述：

`default.custom.yaml`:

```yaml
patch:
  # ...
  schema_list:
    - schema: icemix_<double>
    # ...
```

`radical_pinyin.custom.yaml`:

```yaml
patch:
  # ...
  speller/algebra:
    __include: icemix_<double>.schema.yaml:speller/algebra
```

`melt_eng.custom.yaml`:

```yaml
patch:
  # ...
  speller/algebra:
    __include: melt_eng.schema.yaml:/algebra_<double>
```

`icemix_common.custom.yaml`:

```yaml
patch:
  # ...
  speller/algebra:
    __include: icemix_common:algebra_init
    __append:
      __include: icemix_common:simp_pinyin_<simp>
  translator/dictionary: rime_<dict>
```
