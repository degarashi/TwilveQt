# CXXFLAGS定義の行を探す
/CXXFLAGS\s*=/ {
# もし既にc++11フラグが無ければ追加
/--std=c\+\+/! {
s/=/= --std=c\+\+11 /
}
p
d
}