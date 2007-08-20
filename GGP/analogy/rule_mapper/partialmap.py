from comb_perm import *
import predicate
from GDL import Sentence
import pdb

HEAD_SCORE_FACTOR = 1.5

def possible_matchings(s1, s2):
	# we can't match two instances of the same predicate in one rule
	# to different target predicates
	unique_s1 = list(set(s1))
	unique_s2 = list(set(s2))
	if len(unique_s1) < len(unique_s2):
		return [dict(zip(unique_s1, subset)) for subset in xcombinations(unique_s2, len(unique_s1))]
	else:
		return [dict(zip(subset, unique_s2)) for subset in xcombinations(unique_s1, len(unique_s2))]

def cross_product(l1, l2):
	for a1 in l1:
		for a2 in l2:
			yield (a1, a2)

class PartialMap:

	__place_type_weights = { predicate.AGENT : 2.0,
	                         predicate.OBJECT : 1.0,
	                         predicate.NUM_MIN : 1.5,
	                         predicate.NUM_MAX : 1.5,
	                         predicate.COORD : 0.8,
	                         predicate.NUM : 0.6,
	                         predicate.UNKNOWN : 0.3 }

	@staticmethod
	def create(src_int_rep, src_preds, tgt_int_rep, tgt_preds):
		self = PartialMap()
		# map from predicate name to predicate structure
		self.__src_preds = dict((p.get_name(), p) for p in src_preds)
		self.__tgt_preds = dict((p.get_name(), p) for p in tgt_preds)

		# map all predicates that are identical in source and target onto each other
		common_pred_names = set(self.__src_preds.keys()).intersection(set(self.__tgt_preds.keys()))
		common_pred_map = dict((self.__src_preds[i], self.__tgt_preds[i]) for i in common_pred_names)

		#pdb.set_trace()

		self.__matched_preds = common_pred_map
		self.__matched_rules = {}
		# candidate rule matchings
		# (src_rule, tgt_rule) -> (body map, score)
		self.__cand_rule_matches = {}

		# create all possible matches
		src_rules = [src_int_rep.get_update_rules(), \
		             src_int_rep.get_elab_rules(), \
		             src_int_rep.get_legal_rules(), \
		             src_int_rep.get_goal_rules(), \
		             src_int_rep.get_terminal_rules()]

		tgt_rules = [tgt_int_rep.get_update_rules(), \
		             tgt_int_rep.get_elab_rules(), \
		             tgt_int_rep.get_legal_rules(), \
		             tgt_int_rep.get_goal_rules(), \
		             tgt_int_rep.get_terminal_rules()]
		
		for i in range(5):
			# take all possible pairs of source, target rules
			for sr, tr in cross_product(src_rules[i], tgt_rules[i]):
				# don't include the head predicate in the body, because the head
				# will always matched to the head of the other rule
				srbody = [self.__src_preds[b.get_predicate()] for b in sr.get_body() \
						if b.get_predicate() != sr.get_head().get_predicate()]
				trbody = [self.__tgt_preds[b.get_predicate()] for b in tr.get_body() \
						if b.get_predicate() != tr.get_head().get_predicate()]

				body_maps = possible_matchings(srbody, trbody)
				body_map_scores = [self.__body_map_score(sr, tr, bm) for bm in body_maps]
				# if a body map score == 0, then that map is illegal
				self.__cand_rule_matches[(sr,tr)] = \
						filter(lambda x: x[1] > 0, zip(body_maps, body_map_scores))

		return self
	
	def __init__(self):
		pass

	def copy(self):
		c = PartialMap()
		c.__src_preds = self.__src_preds
		c.__tgt_preds = self.__tgt_preds
		c.__matched_preds = self.__matched_preds.copy()
		c.__matched_rules = self.__matched_rules.copy()
		c.__cand_rule_matches = self.__cand_rule_matches.copy()
		return c

	def __pred_arg_match_score(self, sp, tp):
		if sp.get_arity() == 0 and tp.get_arity() == 0:
			# without any arguments, we can't really compare how "similar"
			# these two predicates are
			return 1
		
		# calculate the score as the number of arguments of the same type
		# divided by the total number of arguments
		intersect = [t for t in sp.get_place_types() if t in tp.get_place_types()]
		#return len(intersect) / float(sp.get_arity() + tp.get_arity())
		if len(intersect) == 0:
			intersect_score = 0
		elif len(intersect) == 1:
			intersect_score = PartialMap.__place_type_weights[intersect[0]]
		else:
			intersect_score = reduce(lambda x,y:x+y, \
					(PartialMap.__place_type_weights[i] for i in intersect))
		return intersect_score / max(abs(sp.get_arity() - tp.get_arity()),1)

	def __pred_match_score(self, sp, tp):
		# check to make sure the predicates don't already have conflicting mappings
		if sp in self.__matched_preds:
			if self.__matched_preds[sp] != tp:
				return 0
			else:
				return 1
		else:
			if tp in self.__matched_preds.values():
				return 0
		
		# ensure that predicate types are mappable
		# legal/does -/-> next/true, elabs
		if Sentence.MOVE in [sp.get_type(), tp.get_type()] and \
				sp.get_type() != tp.get_type():
			return 0
		
		# if all above conditions are met, then make a rating based on argument types
		return self.__pred_arg_match_score(sp, tp) + 0.1

	def __body_map_score(self, sr, tr, m):
		total = 0
		for sp, tp in m.items():
			pm_score = self.__pred_match_score(sp, tp)
			if pm_score == 0:
				# if two predicates are illegal to match, then the
				# entire matching is bad
				return 0
			total += pm_score

		#num_unmatched = max(len(sr.get_body()), len(tr.get_body())) - len(m)
		#return max(0.1, total - num_unmatched) 
		return total

	def __head_match_score(self, sr, tr):
		srhead = sr.get_head()
		trhead = tr.get_head()
		if srhead.get_relation() == 'goal':
			assert trhead.get_relation() == 'goal'
			if srhead.get_term(1) == trhead.get_term(1):
				return 1
			else:
				# this is only assuming that there is at least one
				# 0 score goal and one 100 score goal for each scenario
				return 0
		elif srhead.get_relation() == 'terminal':
			assert trhead.get_relation() == 'terminal'
			return 1
		else:
			shp = self.__src_preds[srhead.get_predicate()]
			thp = self.__tgt_preds[trhead.get_predicate()]
			return self.__pred_match_score(shp, thp)

	def __rule_match_score(self, sr, tr):
		if (sr, tr) not in self.__cand_rule_matches:
			return (None, 0)
		
		# try to matche the head sentences first
		head_match_score = self.__head_match_score(sr, tr)
		if head_match_score == 0:
			# if heads don't match, rules can't match
			return (None, 0)

		# find the best body mapping
		best_score = 0
		best_map = {}
		for body_map, score in self.__cand_rule_matches[(sr,tr)]:
			if score > best_score:
				best_map = body_map
				best_score = score

		best_score += HEAD_SCORE_FACTOR * head_match_score
		return (best_map, best_score)

	def __update_rule_match(self, sr, tr):
		# recalculate body mapping score for all possible mappings
		body_maps = self.__cand_rule_matches[(sr,tr)]
		i = 0
		while i < len(body_maps):
			new_score = self.__body_map_score(sr, tr, body_maps[i][0])
			if new_score == 0:
				# no longer valid map
				del body_maps[i]
			else:
				body_maps[i] = (body_maps[i][0], new_score)
				i += 1
	
	def __update_all_rule_matches(self):
		to_erase = []
		for sr, tr in self.__cand_rule_matches:
			# check if heads still match
			head_match_score = self.__head_match_score(sr, tr)
			if head_match_score == 0:
				to_erase.append((sr, tr))
				continue
			
			self.__update_rule_match(sr, tr)
			if len(self.__cand_rule_matches[(sr,tr)]) == 0:
				to_erase.append((sr,tr))

		for sr, tr in to_erase:
			del self.__cand_rule_matches[(sr,tr)]
		
		return len(to_erase)
	

	def __add_predicate_match(self, sp, tp):
		if sp in self.__matched_preds:
			assert tp == self.__matched_preds[sp]
		else:
			assert tp not in self.__matched_preds.values()
			print "matching predicates %s ==> %s" % (sp, tp)
			self.__matched_preds[sp] = tp
	
	def add_rule_match(self, sr, tr, body_map):
		assert sr not in self.__matched_rules
		assert tr not in self.__matched_rules.values()
		
		print "=== matching rules ==="
		print sr
		print tr
		print "--> predicate matches"
		self.__matched_rules[sr] = tr
		
		# delete all other match candidates involving these rules
		for sr1, tr1 in self.__cand_rule_matches.keys():
			if sr1 == sr or tr1 == tr:
				del self.__cand_rule_matches[(sr1, tr1)]

		# add predicate maps
		# first add head predicates
		spn = sr.get_head().get_predicate()
		tpn = tr.get_head().get_predicate()
		if spn != None and tpn != None:
			sp = self.__src_preds[spn]
			tp = self.__tgt_preds[tpn]
			self.__add_predicate_match(sp, tp)

		# body predicates
		for sp, tp in body_map.items():
			if self.__pred_match_score(sp, tp) >= 1:
				self.__add_predicate_match(sp, tp)

		print "=== done matching rules ==="

		return self.__update_all_rule_matches()

	def get_best_rule_match(self):
		best_score = -1
		best_map = None
		best_sr = None
		best_tr = None
		for sr, tr in self.__cand_rule_matches:
			m, score = self.__rule_match_score(sr, tr)
			if score > best_score:
				best_score = score
				best_map = m
				best_sr = sr
				best_tr = tr
		
		return (best_sr, best_tr, best_map, best_score)
	
	def get_pred_matches(self):
		return self.__matched_preds

	def print_pred_matches(self, file):
		for sp, tp in self.__matched_preds.items():
			file.write('%s %s\n' % (sp.get_name(), tp.get_name()))

	def complete(self):
		return len(self.__cand_rule_matches) == 0

	def suppress_match(self, sr, tr):
		del self.__cand_rule_matches[(sr, tr)]
	
	def score(self):
		score = 0
		for sp, tp in self.__matched_preds.items():
			score += self.__pred_match_score(sp, tp)

		return score

	def src_dof(self, r):
		"""Returns the number of rules r can possibly map to"""
		return len([0 for x in self.__cand_rule_matches if x[0] == r])

	def src_rdof(self, r):
		sr2dof = {}
		max = 0
		for sr, tr in self.__cand_rule_matches:
			if sr in sr2dof:
				sr2dof[sr] += 1
			else:
				sr2dof[sr] = 1
			v = sr2dof[sr]
			if v > max:
				max = v
		if r not in sr2dof:
			return 0
		else:
			return sr2dof[r] / float(max)

	def tgt_dof(self, r):
		"""Returns the number of rules r can possibly map to"""
		return len([0 for x in self.__cand_rule_matches if x[1] == r])

	def tgt_rdof(self, r):
		tr2dof = {}
		max = 0
		for sr, tr in self.__cand_rule_matches:
			if tr in tr2dof:
				tr2dof[tr] += 1
			else:
				tr2dof[tr] = 1
			v = tr2dof[tr]
			if v > max:
				max = v
		if r not in tr2dof:
			return 0
		else:
			return tr2dof[r] / float(max)
