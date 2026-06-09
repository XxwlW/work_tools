"""任务 CRUD 单元测试。"""
from __future__ import annotations

from todolist.services import task_service, category_service, tag_service


def test_add_and_get_task():
    tid = task_service.add_task(title="写周报", priority="A",
                                due_at="2026-12-31 18:00", remind_at="2026-12-31 17:30")
    assert tid > 0
    t = task_service.get(tid)
    assert t is not None
    assert t.title == "写周报"
    assert t.priority == "A"
    assert t.due_at.startswith("2026-12-31 18:00")
    assert t.status == "pending"


def test_empty_title_rejected():
    import pytest
    with pytest.raises(ValueError):
        task_service.add_task(title="   ")


def test_invalid_priority_rejected():
    import pytest
    with pytest.raises(ValueError):
        task_service.add_task(title="x", priority="Z")


def test_invalid_due_rejected():
    import pytest
    with pytest.raises(ValueError):
        task_service.add_task(title="x", due_at="不是日期")


def test_toggle_done():
    tid = task_service.add_task(title="买牛奶")
    task_service.toggle_done(tid)
    t = task_service.get(tid)
    assert t.status == "done"
    assert t.completed_at is not None
    task_service.toggle_done(tid)
    t = task_service.get(tid)
    assert t.status == "pending"
    assert t.completed_at is None


def test_delete_task():
    tid = task_service.add_task(title="临时")
    task_service.remove_task(tid)
    assert task_service.get(tid) is None


def test_category_link():
    cat_id = category_service.add(name="工作", color="#FF0000")
    tid = task_service.add_task(title="开会", category_id=cat_id)
    t = task_service.get(tid)
    assert t.category_id == cat_id


def test_tag_link():
    tag1 = tag_service.get_or_create("工作")
    tag2 = tag_service.get_or_create("重要")
    tid = task_service.add_task(title="任务", tag_ids=[tag1.id, tag2.id])
    t = task_service.get(tid)
    assert set(t.tag_ids) == {tag1.id, tag2.id}


def test_edit_task_changes_fields():
    tid = task_service.add_task(title="原标题", priority="D")
    t = task_service.get(tid)
    t.title = "新标题"
    t.priority = "A"
    task_service.edit_task(t)
    t2 = task_service.get(tid)
    assert t2.title == "新标题"
    assert t2.priority == "A"


def test_list_tasks_with_status_filter():
    task_service.add_task(title="未完成")
    tid = task_service.add_task(title="要完成")
    task_service.toggle_done(tid)
    pending = task_service.list_by_status("pending")
    done = task_service.list_by_status("done")
    assert any(t.title == "未完成" for t in pending)
    assert any(t.title == "要完成" for t in done)
    assert not any(t.title == "未完成" for t in done)
