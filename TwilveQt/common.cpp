#include "common.h"

bool Plan::isFinished() const {
	return (final & current.state) == final;
}
void Plan::_planning() {
	if(isFinished()) {
		// 目標状態にたどり着いたのでこれまでの経路と比較
		if(best.cost > current.cost) {
			best = current;
		}
	} else {
		auto& rc = rcand;
		std::vector<const _ReqActBase*> cact;
		// 一旦選択可能なアクションを選定
		for(auto* act : rc) {
			if(act->canDo(current.state))
				cact.push_back(act);
		}
		// アクション候補から1つ選んで下層の探索
		for(auto* act : cact) {
			auto itr = std::find(rc.begin(), rc.end(), act);
			Q_ASSERT(itr != rc.end());
			// アクションを試す
			PlanHist tmp(act->simulate(current));
			tmp.swap(current);
			// 一時的に除く
			rc.erase(itr);
			// 下層の探索へ
			_planning();
			// ステートと候補を戻す
			rc.push_back(act);
			current.swap(tmp);
		}
	}
}
PlanHist Plan::planning(NeedFlag cur, NeedFlag fnl, const _ReqActBase** acts, size_t nAct) {
	current.state = cur;
	current.cost = 0;
	best.invalidate();

	final = fnl;

	rcand.clear();
	for(size_t i=0 ; i<nAct ; i++)
		rcand.push_back(acts[i]);

	_planning();
	return best;
}
PlanHist::PlanHist() {}
PlanHist::PlanHist(const PlanHist &hist):
	state(hist.state), plan(hist.plan), cost(hist.cost) {}
PlanHist::PlanHist(PlanHist&& hist) {
	swap(hist);
}
PlanHist& PlanHist::operator = (const PlanHist& hist) {
	PlanHist(hist).swap(*this);
	return *this;
}

void PlanHist::swap(PlanHist& hist) noexcept {
	std::swap(cost, hist.cost);
	std::swap(state, hist.state);
	std::swap(plan, hist.plan);
}
void PlanHist::invalidate() {
	cost = std::numeric_limits<decltype(cost)>::max();
}
bool PlanHist::valid() const {
	return cost != std::numeric_limits<decltype(cost)>::max();
}

_ReqActBase::_ReqActBase(NeedFlag need, NeedFlag comp, int cost): _cost(cost), _need(need), _comp(comp) {}
PlanHist _ReqActBase::simulate(const PlanHist& hist) const {
	PlanHist ret;
	ret.cost = hist.cost + _cost;
	ret.state = hist.state | _comp;
	ret.plan = hist.plan;
	ret.plan.push_back(this);
	return std::move(ret);
}
bool _ReqActBase::canDo(State state) const {
	return (state&_need) == _need;
}
NeedFlag _ReqActBase::getComp() const {
	return _comp;
}
