#define DEBUG_TYPE "talkdown"

#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "liberty/Utilities/ModuleLoops.h"
#include "liberty/Talkdown/Tree.h"

#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace llvm;

namespace AutoMP
{

  static std::set<std::pair<std::string, std::string> > getMetadataAsStrings(Instruction *i)
  {
    std::set<std::pair<std::string, std::string> > meta_vec;
    MDNode *meta = i->getMetadata("note.noelle");
    if ( meta )
    {
      LLVM_DEBUG( errs() << "Found noelle metadata for instruction " << *&i << "\n"; );
      auto operands = meta->operands();
      for ( auto &op : operands )
      {
        MDNode *casted_meta = dyn_cast<MDNode>(op.get());
        assert( casted_meta && "Couldn't cast operand to MDNode" );

        MDString *key = dyn_cast<MDString>(casted_meta->getOperand(0));
        MDString *value = dyn_cast<MDString>(casted_meta->getOperand(1));
        assert( key && value && "Couldn't cast key or value from annotation" );

        LLVM_DEBUG( errs() << "key: " << key->getString() << ", value: " << value->getString() << "\n"; );
        meta_vec.emplace(key->getString(), value->getString());
      }
    }

    return meta_vec;
  }

#if 0
  FunctionTree::FunctionTree()
  {
    root = nullptr;
    num_nodes = 0;
  }
#endif

  FunctionTree::FunctionTree(Function *f) : FunctionTree()
  {
  }

  FunctionTree::~FunctionTree()
  {

  }

  Node *FunctionTree::findNodeForLoop( Node *start, Loop *l )
  {
    if ( start->getLoop() == l )
      return start;

    for ( auto &n : start->getChildren() )
    {
      Node *found = findNodeForLoop( n, l );
      if ( found )
        return found;
    }

    return nullptr;
  }

  Node *FunctionTree::findNodeForBasicBlock( Node *start, BasicBlock *bb )
  {
    if ( start->getBB() == bb )
      return start;

    for ( auto &n : start->getChildren() )
    {
      Node *found = findNodeForBasicBlock( n, bb );
      if ( found )
        return found;
    }

    return nullptr;
  }

  Node *searchUpForAnnotation(Node *start, std::pair<std::string, std::string> a)
  {
    while ( start->getParent() )
    {
      for ( auto &annot : start->annotations )
      {
        // if key and value match an annotaion of the current node, return that node
        if ( !annot.get_key().compare(a.first) && !annot.get_value().compare(a.second) )
          return start;
      }

      start = start->getParent();
    }

    return nullptr;
  }

  std::vector<Node *> FunctionTree::getNodesInPreorder(Node *start)
  {
    std::vector<Node *> retval = { start };
    if ( start->getChildren().size() == 0 )
      return retval;
    for ( auto &n : start->getChildren() )
    {
      std::vector<Node *> child_nodes = getNodesInPreorder( n );
      retval.insert( retval.end(), child_nodes.begin(), child_nodes.end() );
    }
    return retval;
  }

  std::vector<Node *> FunctionTree::getAllLoopContainerNodes(void)
  {
    std::vector<Node *> retval;
    std::vector<Node *> all_nodes = getNodesInPreorder( root );
    for ( auto &n : all_nodes )
      if ( n->containsAnnotationWithKey( "__loop_container" ) )
        retval.push_back( n );

    return retval;
  }

  // this function adds loop-container nodes to the tree that will end up being parents to all respective subloops (both
  // subloop-container nodes and basic blocks )
  void FunctionTree::addLoopsToTree(LoopInfo &li)
  {
    std::set<Loop *> visited_loops;
    auto loops = li.getLoopsInPreorder(); // this shit is amazing!

    for ( auto const &l : loops )
    {
      Node *tmp = new Node();

      // add loops at level 1 manually
      if ( l->getParentLoop() == nullptr )
      {
        tmp->setParent( root );
        root->addChild( tmp );
      }

      // find the node containing parent loop and add as child to that node
      else
      {
        Node *tmp1 = findNodeForLoop( root, l->getParentLoop() ); // pretty stupid but I'll change it later maybe
        assert( tmp != nullptr && "Subloop doesn't have a parent loop -- something is wrong with getLoopsInPreorder()" );
        tmp->setParent( tmp1 );
        tmp1->addChild( tmp );
      }

      // add a temporary annotation
      tmp->annotations.emplace( l, "__loop_container", "yes" );
      tmp->annotations.emplace( l, "__level", std::to_string(l->getLoopDepth()) );
      tmp->setLoop( l );
      tmp->setID( num_nodes );
      visited_loops.insert( l );
      num_nodes++;
      nodes.push_back( tmp ); // BAD: remove once iterator done
    }

    // Smart thing to do would be to annotate each node as we add it so we don't have to find the node again but
    // that's for later. Let's do something dumb for now...
  }

  // This function is pretty stupid right now as FunctionTree doesn't have an iterator yet.
  // Seems like metadata is only attached to branch instruction in loop header and not the icmp instruction before...
  // Should be called immediately and only after addLoopsToTree()
  void FunctionTree::annotateLoops()
  {
    // traverse nodes in tree
    for ( auto &node : nodes )
    {
      Loop *l = node->getLoop();

      // skip if node is not a loop container
      if ( !l )
      {
        /* errs() << "Node with ID " << node->getID() << " doesn't have an associated loop\n"; */
        continue;
      }

      BasicBlock *header = l->getHeader();
      assert( header && "Loop doesn't have header!" );

      // for each instruction in loop header, check if it has noelle metadata attached. If so, add the annotation
      // to the loop container node
      for ( auto &i : *header )
      {
        auto meta_vec = getMetadataAsStrings( &i );
        for ( auto const &meta_pair : meta_vec )
          node->annotations.emplace( l, meta_pair.first, meta_pair.second );
      }
    }
  }

  // should be called after annotateLoops()
  void FunctionTree::addBasicBlocksToLoops(LoopInfo &li)
  {

    for ( auto &bb : *associated_function )
    {
      Loop *l = li.getLoopFor( &bb );
      if ( !l )
        continue;

      Node *insert_pt = findNodeForLoop( root, l );
      assert( insert_pt && "No node found for loop" );

      Node *tmp = new Node();
      tmp->setBB( &bb );
      tmp->setParent( insert_pt );
      insert_pt->addChild( tmp );
      tmp->setID( num_nodes );
      tmp->annotations.emplace(nullptr, "__loop_bb", "true");
      if ( l->getHeader() == &bb )
        tmp->annotations.emplace(nullptr, "__loop_header", "true");
      num_nodes++;
    }
  }

  void FunctionTree::backAnnotateLoopFromBasicBlocks(Loop *l)
  {

  }

  // need to find out which loop the critical annotation corresponds to
  // returns true if modified
  bool FunctionTree::handleCriticalAnnotations(void)
  {
    bool modified = false;
    std::vector<Instruction *> with_critical; // split points that start a critical section
    std::vector<Instruction *> without_critical; // split points that come out of a critical section
    std::set<BasicBlock *> invalidated_bbs;

    // step 1: split basic blocks where annotation only applies to some of the instructions
    for ( auto &bb : *associated_function )
    {
      bool found_crit = false;
      for ( auto &i : bb )
      {
        auto meta = getMetadataAsStrings( &i );

        // if previous instruction didn't have critical annotation and this one does, split the basic block here!
        if ( meta.find(std::pair<std::string, std::string>("critical", "1")) !=  meta.end() && !found_crit )
        {
          // make sure to add the new block to the loop!
          modified = true;
          with_critical.push_back( &i );
          invalidated_bbs.insert( i.getParent() );
        }

        // if previous instruction had critical annotation and this one doesn't, split the basic block here!
        else if ( meta.find(std::pair<std::string, std::string>("critical", "1")) ==  meta.end() && found_crit )
        {
          modified = true;
          without_critical.push_back( &i );
          invalidated_bbs.insert( i.getParent() );
        }
      }
    }

    // if we didn't have to split basic blocks, don't continue with the rest
    if ( !modified )
      return false;

    // remove basic blocks that have been invalidated and free the node
    // Basic block nodes can't have any children so don't need to worry about that
    /* for ( auto &bb : invalidated_bbs ) */
    /* { */
    /*   Node *n = findNodeForBasicBlock( root, bb ); */
    /*   (n->parent)->removeChild( n ); */
    /*   delete n; */
    /* } */

    for ( auto &i : with_critical )
    {
      Node *n = findNodeForBasicBlock( root, i->getParent() );
      BasicBlock *new_block = SplitBlock( i->getParent(), i );

      Node *new_node = new Node();
      new_node->annotations.emplace( nullptr, "critical", "1" );
      new_node->setParent( n->getParent() );
      new_node->setBB( new_block );
      (n->getParent())->addChild( new_node );
      new_node->setID( num_nodes );
      num_nodes++;
    }

    for ( auto &i : with_critical )
    {
      Node *n = findNodeForBasicBlock( root, i->getParent() );
      BasicBlock *new_block = SplitBlock( i->getParent(), i );

      Node *new_node = new Node();
      new_node->annotations.emplace( nullptr, "critical", "0" );
      new_node->setParent( n->getParent() );
      new_node->setBB( new_block );
      (n->getParent())->addChild( new_node );
      new_node->setID( num_nodes );
      num_nodes++;
    }

    return true;
  }

  bool FunctionTree::handleOwnedAnnotations(void)
  {
  }

  // construct a tree for each function in program order
  // steps:
  //  1. Basic blocks that don't belong to any loops don't have any annotations. They should be direct children of the root node
  //  2. Create container nodes for each outer loop, with root as a parent
  //  3. For each subloop, create a container node that a child of the parent loop
  //  4. Annotate each loop with annotations from its header basic block
  bool FunctionTree::constructTree(Function *f, LoopInfo &li)
  {
    bool modified = false;
    std::set<BasicBlock *> visited_bbs;

    associated_function = f;

    // construct root node
    // TODO(greg): with annotation of function if there is one
    this->root = new Node();
    root->setParent( nullptr );
    root->annotations.emplace( nullptr, "__root", "yes" );
    root->setID( 0 );
    num_nodes++;
    nodes.push_back( root ); // BAD: remove once iterator done

    addLoopsToTree( li );
    annotateLoops();

    for ( auto &bb : *f )
    {
      // skip bbs that are already handled
      if ( visited_bbs.find( &bb ) != visited_bbs.end() )
        continue;

      Loop *l = li.getLoopFor( &bb );

      // Basic blocks that don't belong to any loop
      if ( !l )
      {
        Node *tmp = new Node();
        tmp->setParent( root );
        root->addChild( tmp );
        tmp->annotations.emplace( nullptr, "__non_loop_bbs", "true" );
        tmp->setID( num_nodes );
        visited_bbs.insert( &bb );
        num_nodes++;
        nodes.push_back( tmp ); // BAD: remove once iterator done
      }
    }

    addBasicBlocksToLoops( li );

    modified |= handleCriticalAnnotations();

    return modified;
  }

#if 0
  SESENode *FunctionTree::getInnermostNode(Instruction *inst)
  {

  }

  SESENode *FunctionTree::getParent(SESENode *node)
  {
    return node->getParent();
  }

  SESENode *FunctionTree::getFirstCommonAncestor(SESENode *n1, SESENode *n2)
  {
    SESENode *deeper;
    SESENode *shallower;

    // if same node, return one of them
    if ( n1 == n2 )
      return n1;

    // find which node is deeper in the tree
    if ( n1->getDepth() > n2->getDepth() )
    {
      deeper = n1;
      shallower = n2;
    }
    else
    {
      deeper = n2;
      shallower = n1;
    }

    // get the two search nodes to the same depth
    while ( shallower->getDepth() != deeper->getDepth() )
      deeper = deeper->getParent();

    while ( deeper != root )
    {
      if ( deeper == shallower )
        return deeper;

      deeper = deeper->getParent();
      shallower = shallower->getParent();
    }

    // if we get to root, then there was no common ancestor
    return nullptr;
  }
#endif


  void FunctionTree::writeDotFile( const std::string filename )
  {
    // NOTE(greg): Use GraphWriter to print out the tree better
  }

  std::ostream &operator<<(std::ostream &os, const FunctionTree &tree)
  {
    os << "Tree for function " << tree.associated_function->getName().str() << ":\n";
    return tree.root->recursivePrint( os );
  }

} // namespace llvm
