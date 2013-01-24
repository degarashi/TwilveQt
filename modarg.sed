# CXXFLAGSの行を探す
/CXXFLAGS\s*=/ {
	# かつ -Wall の行を探す
	/-Wall/ {
		# まだ -Wunusedがなければ追加
		/-Wno-unused-parameter/! {
			s/\(-Wall\)/\1 -Wno-unused-parameter -Wno-unused-variable/
		}
	}
}
p
d
