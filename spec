Problem:
・JDocがArrayなら分解してからreceiveInfo()に渡す
・planning結果はキャッシュする std::map<uint64_t((RequestFlag<<32) | HaveF), std::vector<const ReqAct*>>

同じタイプの要求をレスポンスが帰ってくるまでに2回以上して、後に古いResが帰ってきた場合に無視する